#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "glex.h"
#include "mpi.h"

#define TEST_Z(x,y) do { if (!(x)) die(y); } while (0)

char *mem_addr;

static int die(const char *reason){
	fprintf(stderr, "Err: %s - %s\n ", strerror(errno), reason);
	exit(EXIT_FAILURE);
	return -1;
}

// 如果glex_ret_t的返回值非success，则打印错误
static void TEST_RetSuccess(glex_ret_t glex_ret, char* str){
	if ((strcmp(glex_error_str(glex_ret), "successful") != 0)) {
		printf("Err: %s——%s\n", glex_error_str(glex_ret), str);
		exit(EXIT_FAILURE);
		return; 
	}
}

/*
 * 函数：配置GLEX环境,初始化glex相关网卡信息
 * 参数：
 * 返回值：nic_id
 */
static int init_glex_env(glex_device_handle_t *init_dev, glex_ep_handle_t *init_ep, struct glex_ep_attr* init_ep_attr, glex_ep_addr_t *init_ep_addr, 
	uint32_t *nicID, uint32_t *EPNum){
	// 所有的glex用户接口函数返回该类型值
	glex_ret_t ret;
	// GLEX设备的数量
	uint32_t num_of_devices;

	/* 获得GLEX设备的数量 */
	glex_num_of_device(&num_of_devices);

	/* 打开GLEX设备 */
	ret = glex_open_device(0, init_dev);	//dev用于储存被打开设备的标识符
	TEST_RetSuccess(ret, "0号设备通信接口打开失败！");

	/* 获得已打开接口设备的属性信息 */
	struct glex_device_attr dev_attr;
	ret = glex_query_device(*init_dev, &dev_attr);
	TEST_RetSuccess(ret, "获取通信接口属性信息失败！");

	/* 在通信接口上创建一个端点，作为虚拟化硬件通信资源的软件实体 */
	(*init_ep_attr).type = GLEX_EP_TYPE_FAST;
	(*init_ep_attr).mpq_type = GLEX_MPQ_TYPE_NORMAL;
	(*init_ep_attr).eq_type = GLEX_EQ_TYPE_NORMAL;
	(*init_ep_attr).num = GLEX_ANY_EP_NUM;
	(*init_ep_attr).key = 13;
	//.dq_capacity = 1024;
	//.mpq_capacity = 1024;
	//.eq_capacity = 1024;
	ret = glex_create_ep(*init_dev, init_ep_attr, init_ep);

	/* 获得指定端点的端点地址 */
	// 本地端点地址
	ret = glex_get_ep_addr(*init_ep, init_ep_addr);

	/* 获得一个端点的相关属性信息 */
	ret = glex_query_ep(*init_ep, init_ep_attr);

	/* 构造端点的地址 */
	glex_compose_ep_addr(dev_attr.nic_id, (*init_ep_attr).num, (*init_ep_attr).type, init_ep_addr);
	
	/* 显示端点所在通信接口编号、端点序号 */
	glex_decompose_ep_addr(*init_ep_addr, (*init_ep_attr).type, nicID, EPNum);

	return 0;
}


int main(int argc, char *argv[])
{
	/**** 初始化MPI环境 ****/
	int num_procs,my_id;		//进程总数、进程ID
	MPI_Status status;			//status对象(Status)
	MPI_Request handle;			//MPI请求(request)

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&num_procs);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_id);

	TEST_Z(num_procs == 2,
          "本程序需要且仅需要两个进程(发送和接收)。");

	/**** 配置GLEX环境 ****/
	glex_ret_t ret;
	glex_device_handle_t dev;
	glex_ep_handle_t ep;
	struct glex_ep_attr ep_attr = {};
	glex_ep_addr_t ep_addr;
	uint32_t nicID, EPNum;
	init_glex_env(&dev,&ep,&ep_attr,&ep_addr,&nicID,&EPNum);

	/* 注册内存，锁内存并建立映射关系 */
	char *mem_addr = (char *)malloc(sizeof(char)*20);
	glex_mem_handle_t mh;
	ret = glex_register_mem(ep, mem_addr, sizeof(char)*20, GLEX_MEM_READ|GLEX_MEM_WRITE, &mh);

	/****
			交换两节点的端点地址信息（glex_ep_addr_t）
	*****/
	/* 定义本地端点地址和远程端点地址的char数组，便于MPI发送 */
	char endpoint_info[num_procs][sizeof "0000:0000:00000000:0000000000000000"];
	sprintf(endpoint_info[my_id], "%04x:%04x:%08x:%016Lx", 
				ep_addr.s.ep_num, ep_addr.s.resv, ep_addr.s.nic_id, ep_addr.v);

	/* 利用MPI，交换发送、接收端点的地址(glex_ep_addr_t) */ 
	MPI_Request  send_request[num_procs];
    MPI_Request  recv_request[num_procs];
	int procTemp;
	/* 防止死锁，使用异步发送和接收交换端点间信息 */ 
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		printf("进程%d发送给进程%d\n", my_id, procTemp);
		MPI_Isend(endpoint_info[my_id], sizeof(endpoint_info[my_id]), MPI_CHAR, procTemp, 0, MPI_COMM_WORLD, &send_request[procTemp]);
    	MPI_Irecv(endpoint_info[procTemp], sizeof(endpoint_info[procTemp]), MPI_CHAR, procTemp,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp]);
	}
	/* 等待异步交换信息过程完成 */ 
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		MPI_Wait(&send_request[procTemp],&status);
		MPI_Wait(&recv_request[procTemp],&status);
	}

	/* 将接收的各个远程端点的地址信息数据存储于remote_ep_addr中 */
	glex_ep_addr_t remote_ep_addr[num_procs];
	int parsed = 0;		// 用于验证接收后参数数量是否正确
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		printf("进程%d发送给进程%d\n", my_id, procTemp);
		parsed = sscanf(endpoint_info[procTemp], "%x:%x:%x:%Lx", 
					&(remote_ep_addr[procTemp].s.ep_num), &(remote_ep_addr[procTemp].s.resv), &(remote_ep_addr[procTemp].s.nic_id), &(remote_ep_addr[procTemp].v));
		// 验证接收过程和存储过程是否正常
		if(parsed != 4)
			fprintf(stderr, "进程%d发送和接收端点交换信息失败", procTemp);
		printf("进程%d————本地端点NIC ID：%d, 端点序号: %d, 远程端点NIC ID：%d, 远程端点序号：%d\n", procTemp, nicID, EPNum, remote_ep_addr[procTemp].s.nic_id, remote_ep_addr[procTemp].s.ep_num);
	}

	/****
			交换两节点的内存标识信息（glex_mem_handle_t）
	*****/
	/* 定义本地端点地址和远程端点地址的char数组(交换内存标识信息)，便于MPI发送 */
	sprintf(endpoint_info[my_id], "%04x:%04x:%08x:%016Lx", 
				mh.s.mmt_index, mh.s.att_base_off, mh.s.att_index, mh.v);

	/* 利用MPI，交换发送、接收端点的地址(glex_mem_handle_t) */ 
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		MPI_Isend(endpoint_info[my_id], sizeof(endpoint_info[my_id]), MPI_CHAR, procTemp, 0, MPI_COMM_WORLD, &send_request[procTemp]);
    	MPI_Irecv(endpoint_info[procTemp], sizeof(endpoint_info[procTemp]), MPI_CHAR, procTemp,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp]);
	} 
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		MPI_Wait(&send_request[procTemp],&status);
		MPI_Wait(&recv_request[procTemp],&status);
	}

	/* 将接收的数据存储于remote_mem_addr中 */
	// 存储远程端点内存标识
	glex_mem_handle_t remote_mem_addr[num_procs];
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		parsed = sscanf(endpoint_info[procTemp], "%x:%x:%x:%Lx", 
					&(remote_mem_addr[procTemp].s.mmt_index), &(remote_mem_addr[procTemp].s.att_base_off), &(remote_mem_addr[procTemp].s.att_index), &(remote_mem_addr[procTemp].v));
		// 验证接收过程和存储过程是否正常
		if(parsed != 4){
			fprintf(stderr, "发送和接收端点交换信息失败");
			return -1;
		}
		printf("本地端点mmt_index：%d, 远程端点mmt_index：%d\n", mh.s.mmt_index, remote_mem_addr[procTemp].s.mmt_index);
	}

	/* 利用GLEX，在本地端点与远程端点之间，进行RDMA通信操作 */
	if(my_id == 0){
		strcpy(mem_addr, "Hello!\n");

		// 触发的远程事件，便于被写节点知道RDMA写已完成
		struct glex_event remote_event = {
			.cookie_0 = 10,
			.cookie_1 = 11
		};

		struct glex_rdma_req rdmaReq = {
			.rmt_ep_addr = remote_ep_addr[1],
			.local_mh = mh,
			.local_offset = 0,
			.len = 7,
			.rmt_mh = remote_mem_addr[1],
			.rmt_offset = 0,
			.type = GLEX_RDMA_TYPE_PUT,
//			.local_evt = ,
			.rmt_evt = remote_event,
			.rmt_key = 13,			// ep_attr初始化时设置的
//			.coll_counter = ,
//			.coll_set_num = ,
			.flag = GLEX_FLAG_REMOTE_EVT,
			.next = NULL
		};

		// 发送RDMA PUT通信请求
		ret = glex_rdma(ep, &rdmaReq, NULL);
		TEST_RetSuccess(ret, "非阻塞RDMA写失败！");

		// 轮询检查RDMA操作是否出现错误请求
		uint32_t num_er;
		struct glex_err_req *er_list;
		ret = glex_poll_error_req(ep, &num_er, er_list);
		if(num_er != 0)
			printf("%s\n", "glex_rdma操作失败，num_er不为0。");
	}else{
		glex_event_t *event = (glex_event_t *)malloc(10*sizeof(glex_event_t));
		ret = glex_probe_first_event(ep, -1, &event);
		TEST_RetSuccess(ret, "被写端点未接收到触发事件！");
		printf("cookie_0:%d, cookie_1:%d\n", event[0].cookie_0, event[0].cookie_1);
		printf("接收节点：接收后，buffer内容是，%s\n", mem_addr);
	}


	/* 其他工作… */



	MPI_Finalize();

	/* 释放内存 */
	ret = glex_deregister_mem(ep, mh);
	free(mem_addr);

	/* 释放端点资源 */
	ret = glex_destroy_ep(ep);

	/* 关闭GLEX设备 */
	ret = glex_close_device(dev);

	return 0;
}
