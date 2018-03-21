#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "glex.h"
#include "mpi.h"

static int die(const char *reason){
	fprintf(stderr, "Err: %s - %s\n ", strerror(errno), reason);
	exit(EXIT_FAILURE);
	return -1;
}

#define TEST_Z(x,y) do { if (!(x)) die(y); } while (0)

// 如果glex_ret_t的返回值非success，则打印错误
static void TEST_RetSuccess(glex_ret_t glex_ret, char* str){
	if ((strcmp(glex_error_str(glex_ret), "successful") != 0)) {
		printf("Err: %s——%s\n", glex_error_str(glex_ret), str);
		exit(EXIT_FAILURE);
		return; 
	}
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
	// 所有的glex用户接口函数返回该类型值
	glex_ret_t ret;
	// GLEX设备的数量
	uint32_t num_of_devices;
	// 用户定义的变量地址，用于储存互连通信接口设备的标识符
	glex_device_handle_t dev;

	/* 获得GLEX设备的数量 */
	glex_num_of_device(&num_of_devices);
	printf("共有%d个GLEX设备。\n", num_of_devices);

	/* 打开GLEX设备 */
	ret = glex_open_device(0, &dev);	//dev用于储存被打开设备的标识符
	TEST_RetSuccess(ret, "0号设备通信接口打开失败！");

	/* 获得已打开接口设备的属性信息 */
	struct glex_device_attr dev_attr;
	ret = glex_query_device(dev, &dev_attr);
	TEST_RetSuccess(ret, "获取通信接口属性信息失败！");

	/* 在通信接口上创建一个端点，作为虚拟化硬件通信资源的软件实体 */
	glex_ep_handle_t ep;
	struct glex_ep_attr ep_attr = {
		.type = GLEX_EP_TYPE_FAST,
		.mpq_type = GLEX_MPQ_TYPE_NORMAL,
		.eq_type = GLEX_EQ_TYPE_NORMAL,
		.num = GLEX_ANY_EP_NUM,
		.key = 13
		//.dq_capacity = 1024;
		//.mpq_capacity = 1024;
		//.eq_capacity = 1024;
	};
	ret = glex_create_ep(dev, &ep_attr, &ep);

	/* 获得指定端点的端点地址 */
	// 本地端点地址
	glex_ep_addr_t ep_addr;
	ret = glex_get_ep_addr(ep, &ep_addr);

	/* 获得一个端点的相关属性信息 */
	ret = glex_query_ep(ep, &ep_attr);

	/* 构造端点的地址 */
	glex_compose_ep_addr(dev_attr.nic_id, ep_attr.num, ep_attr.type, &ep_addr);
	
	/* 显示端点所在通信接口编号、端点序号 */
	uint32_t nicID, EPNum;
	glex_decompose_ep_addr(ep_addr, ep_attr.type, &nicID, &EPNum);

	/* 注册内存，锁内存并建立映射关系 */
	char *mem_addr = (char *)malloc(sizeof(char)*10);
	glex_mem_handle_t mh;
	ret = glex_register_mem(ep, mem_addr, sizeof(char)*10, GLEX_MEM_READ|GLEX_MEM_WRITE, &mh);

	/* 定义本地端点地址和远程端点地址的char数组，便于MPI发送 */
	char msg[sizeof "0000:0000:00000000:0000000000000000"];
	char remoteMsg[sizeof "0000:0000:00000000:0000000000000000"];
	int parsed;		// 用于验证接收后参数数量是否正确
	sprintf(msg, "%04x:%04x:%08x:%016Lx", 
				ep_addr.s.ep_num, ep_addr.s.resv, ep_addr.s.nic_id, ep_addr.v);

	/* 利用MPI，交换发送、接收端点的地址(glex_ep_addr_t) */
	if(my_id == 0){		//发送进程
		MPI_Isend(msg, strlen(msg), MPI_CHAR, 1, 0, MPI_COMM_WORLD, &handle);
		MPI_Recv(remoteMsg, strlen(msg), MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status);
	}else{				//接收进程
		MPI_Isend(msg, strlen(msg), MPI_CHAR, 0, 0, MPI_COMM_WORLD, &handle);
		MPI_Recv(remoteMsg, strlen(msg), MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
	}

	/* 将接收的数据存储于remote_ep_addr中 */
	// 远程端点地址
	glex_ep_addr_t remote_ep_addr;
	parsed = sscanf(remoteMsg, "%x:%x:%x:%Lx", 
					&(remote_ep_addr.s.ep_num), &(remote_ep_addr.s.resv), &(remote_ep_addr.s.nic_id), &(remote_ep_addr.v));
	// 验证接收过程和存储过程是否正常
	if(parsed != 4)
		fprintf(stderr, "发送和接收端点交换信息失败");

	printf("本地端点NIC ID：%d, 端点序号: %d, 远程端点NIC ID：%d\n", nicID, EPNum, remote_ep_addr.s.nic_id);

	/* 利用GLEX，以非阻塞的方式，提交一个MP报文的发送 */
	if(my_id == 0){
		strcpy(mem_addr, "Hello!\n");
		struct glex_imm_mp_req mpReq = {
			.rmt_ep_addr = remote_ep_addr,
			.data = mem_addr,
			.len = sizeof(char) * 10,
			.next = NULL
		};
		ret = glex_send_imm_mp(ep, &mpReq, NULL);
		TEST_RetSuccess(ret, "非阻塞发送MP报文失败！");
	}else{
		glex_ep_addr_t source_ep_addr;
		uint32_t mpLen;
		printf("\n接收节点：接收前，buffer内容是：%s\n", mem_addr);
		ret = glex_receive_mp(ep, 30, &source_ep_addr, mem_addr, &mpLen);
		TEST_RetSuccess(ret, "阻塞接收MP报文失败！");
		printf("接收节点：接收后，buffer内容是，%s\n", mem_addr);
	}


	/* 其他工作… */


	/* 释放内存 */
	ret = glex_deregister_mem(ep, mh);
	free(mem_addr);

	/* 释放端点资源 */
	ret = glex_destroy_ep(ep);

	/* 关闭GLEX设备 */
	ret = glex_close_device(dev);

	return 0;
}
