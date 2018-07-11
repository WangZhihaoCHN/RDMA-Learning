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
	int ierr;					//MPI返回值
	double inittime,recvtime,totaltime;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&num_procs);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_id);

	/* 从程序运行参数中获得二维进程矩阵边长和buffer的大小 */
    int row=atoi(argv[1]);			//行
    int col=atoi(argv[2]);			//列
    int buffsize=atoi(argv[3]);

	/**** 配置GLEX环境 ****/
	glex_ret_t ret;
	glex_device_handle_t dev;
	glex_ep_handle_t ep;
	struct glex_ep_attr ep_attr = {};
	glex_ep_addr_t ep_addr;
	uint32_t nicID, EPNum;
	init_glex_env(&dev,&ep,&ep_attr,&ep_addr,&nicID,&EPNum);

	/* 注册内存，锁内存并建立映射关系 */
	int memTmp = 0;
    double **sendbuff=(double **)malloc(4*sizeof(double*));
	for(memTmp=0;memTmp<4;++memTmp)
		sendbuff[memTmp] = (double*)malloc(buffsize*sizeof(double));
    double **recvbuff = (double **)malloc(4*sizeof(double*));
	for(memTmp=0;memTmp<4;++memTmp)
		recvbuff[memTmp] = (double*)malloc(buffsize*sizeof(double));

	glex_mem_handle_t mh[4];
	// 存储远程端点内存标识
	glex_mem_handle_t remote_mem_addr[4];
	for(memTmp=0;memTmp<4;++memTmp){
		ret = glex_register_mem(ep, sendbuff[memTmp], sizeof(double)*buffsize, GLEX_MEM_READ|GLEX_MEM_WRITE, &mh[memTmp]);
		ret = glex_register_mem(ep, recvbuff[memTmp], sizeof(double)*buffsize, GLEX_MEM_READ|GLEX_MEM_WRITE, &remote_mem_addr[memTmp]);
	}

	/****
			交换两节点的端点地址信息（glex_ep_addr_t）
	*****/
	/* 定义本地端点地址和远程端点地址的char数组，便于MPI发送 */
	char endpoint_info[num_procs][sizeof "0000:0000:00000000:0000000000000000"];
	sprintf(endpoint_info[my_id], "%04x:%04x:%08x:%016Lx", 
				ep_addr.s.ep_num, ep_addr.s.resv, ep_addr.s.nic_id, ep_addr.v);

	/* 利用MPI，交换发送、接收端点的地址(glex_ep_addr_t) */ 
	MPI_Request  send_request[num_procs][4];
    MPI_Request  recv_request[num_procs][4];
	int procTemp;
	/* 防止死锁，使用异步发送和接收交换端点间信息 */ 
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		MPI_Isend(endpoint_info[my_id], sizeof(endpoint_info[my_id]), MPI_CHAR, procTemp, 0, MPI_COMM_WORLD, &send_request[procTemp][0]);
    	MPI_Irecv(endpoint_info[procTemp], sizeof(endpoint_info[procTemp]), MPI_CHAR, procTemp,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp][0]);
	}
	/* 等待异步交换信息过程完成 */ 
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		MPI_Wait(&send_request[procTemp][0],&status);
		MPI_Wait(&recv_request[procTemp][0],&status);
	}

	/* 将接收的各个远程端点的地址信息数据存储于remote_ep_addr中 */
	glex_ep_addr_t remote_ep_addr[num_procs];
	int parsed = 0;		// 用于验证接收后参数数量是否正确
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		//printf("进程%d发送给进程%d\n", my_id, procTemp);
		parsed = sscanf(endpoint_info[procTemp], "%x:%x:%x:%Lx", 
					&(remote_ep_addr[procTemp].s.ep_num), &(remote_ep_addr[procTemp].s.resv), &(remote_ep_addr[procTemp].s.nic_id), &(remote_ep_addr[procTemp].v));
		// 验证接收过程和存储过程是否正常
		if(parsed != 4)
			fprintf(stderr, "进程%d发送和接收端点交换信息失败", procTemp);
		// printf("本地%d————本地端点NIC ID：%d, 端点序号: %d \n远程%d————远程端点NIC ID：%d, 远程端点序号：%d\n", my_id, nicID, EPNum, procTemp,remote_ep_addr[procTemp].s.nic_id, remote_ep_addr[procTemp].s.ep_num);
	}
	printf("本地%d————本地端点NIC ID：%d, 端点序号: %d\n", my_id, nicID, EPNum);

	/****
			交换两节点的内存标识信息（glex_mem_handle_t）
	*****/
	/* 定义本地端点地址和远程端点地址的char数组(交换内存标识信息)，便于MPI发送 */
	// 0-left/1-right/2-up/3-down
	char local[4][sizeof "0000:0000:00000000:0000000000000000"];
	char remote[4][sizeof "0000:0000:00000000:0000000000000000"];
	for(procTemp=0; procTemp<4; procTemp++)
		sprintf(local[procTemp], "%04x:%04x:%08x:%016Lx", 
					mh[procTemp].s.mmt_index, mh[procTemp].s.att_base_off, mh[procTemp].s.att_index, mh[procTemp].v);
	/* 利用MPI，交换发送、接收端点的地址(glex_mem_handle_t) */ 
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
    	// 向左进程发送与接收
	    if(my_id != 0 && (my_id-1)/col == my_id/col){
	    	//printf("rank %d 的左进程为 %d\n", my_id, my_id-1);
	    	ierr = MPI_Isend(local[0],sizeof(local[0]),MPI_CHAR,my_id-1,0,MPI_COMM_WORLD,&send_request[procTemp][0]);
	    	ierr = MPI_Irecv(remote[0],sizeof(remote[0]),MPI_CHAR,my_id-1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp][0]);
	    }else{
	    	//printf("rank %d 的左进程为 %d\n", my_id, my_id+col-1);
	    	ierr = MPI_Isend(local[0],sizeof(local[0]),MPI_CHAR,my_id+col-1,0,MPI_COMM_WORLD,&send_request[procTemp][0]);
	    	ierr = MPI_Irecv(remote[0],sizeof(remote[0]),MPI_CHAR,my_id+col-1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp][0]);
	    }
	    // 向右进程发送与接收
	    if((my_id+1)/col == my_id/col){
	    	//printf("rank %d 的右进程为 %d\n", my_id, my_id+1);
	    	ierr = MPI_Isend(local[1],sizeof(local[1]),MPI_CHAR,my_id+1,0,MPI_COMM_WORLD,&send_request[procTemp][1]);
	    	ierr = MPI_Irecv(remote[1],sizeof(remote[1]),MPI_CHAR,my_id+1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp][1]);
	    }else{
	    	//printf("rank %d 的右进程为 %d\n", my_id, my_id-col+1);
	    	ierr = MPI_Isend(local[1],sizeof(local[1]),MPI_CHAR,my_id-col+1,0,MPI_COMM_WORLD,&send_request[procTemp][1]);
	    	ierr = MPI_Irecv(remote[1],sizeof(remote[1]),MPI_CHAR,my_id-col+1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp][1]);
	    }
	    // 向上进程发送与接收
	    if((my_id-col)>=0){
	    	//printf("rank %d 的上进程为 %d\n", my_id, my_id-col);
	    	ierr = MPI_Isend(local[2],sizeof(local[2]),MPI_CHAR,my_id-col,0,MPI_COMM_WORLD,&send_request[procTemp][2]);
	    	ierr = MPI_Irecv(remote[2],sizeof(remote[2]),MPI_CHAR,my_id-col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp][2]);
	    }else{
	    	//printf("rank %d 的上进程为 %d\n", my_id, my_id+(row-1)*col);
	    	ierr = MPI_Isend(local[2],sizeof(local[2]),MPI_CHAR,my_id+(row-1)*col,0,MPI_COMM_WORLD,&send_request[procTemp][2]);
	    	ierr = MPI_Irecv(remote[2],sizeof(remote[2]),MPI_CHAR,my_id+(row-1)*col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp][2]);
	    }
	    // 向下进程发送与接收
	    if((my_id+col)<num_procs){
	    	//printf("rank %d 的下进程为 %d\n", my_id, my_id+col);
	    	ierr = MPI_Isend(local[3],sizeof(local[3]),MPI_CHAR,my_id+col,0,MPI_COMM_WORLD,&send_request[procTemp][3]);
	    	ierr = MPI_Irecv(remote[3],sizeof(remote[3]),MPI_CHAR,my_id+col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp][3]);
	    }else{
	    	//printf("rank %d 的下进程为 %d\n", my_id, my_id-(row-1)*col);
	    	ierr = MPI_Isend(local[3],sizeof(local[3]),MPI_CHAR,my_id-(row-1)*col,0,MPI_COMM_WORLD,&send_request[procTemp][3]);
	    	ierr = MPI_Irecv(remote[3],sizeof(remote[3]),MPI_CHAR,my_id-(row-1)*col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[procTemp][3]);
	    }
	} 
	// 等待执行完成
	int fx = 0;
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		for(fx=0; fx<4; fx++){
			MPI_Wait(&send_request[procTemp][fx],&status);
			MPI_Wait(&recv_request[procTemp][fx],&status);
		}
	}

	/* 将接收的数据存储于remote_mem_addr中 */
	for(procTemp=0; procTemp<num_procs; procTemp++){
		if(procTemp == my_id)
			continue;
		for(fx=0; fx<4; fx++){
			parsed = sscanf(remote[fx], "%x:%x:%x:%Lx", 
						&(remote_mem_addr[fx].s.mmt_index), &(remote_mem_addr[fx].s.att_base_off), &(remote_mem_addr[fx].s.att_index), &(remote_mem_addr[fx].v));
			// 验证接收过程和存储过程是否正常
			if(parsed != 4){
				fprintf(stderr, "发送和接收端点交换信息失败");
				return -1;
			}
			// printf("本地端点mmt_index：%d, 远程端点mmt_index：%d\n", mh[fx].s.mmt_index, remote_mem_addr[fx].s.mmt_index);
		}
	}

	// 触发的远程事件，便于被写节点知道RDMA写已完成
	struct glex_event remote_event0 = {
		.cookie_0 = 10,
		.cookie_1 = 11
	};
	struct glex_event remote_event1 = {
		.cookie_0 = 11,
		.cookie_1 = 12
	};
	struct glex_event remote_event2 = {
		.cookie_0 = 12,
		.cookie_1 = 13
	};
	struct glex_event remote_event3 = {
		.cookie_0 = 13,
		.cookie_1 = 14
	};


	// 判断当前结点的上下左右四个结点分别的进程号
	int up,down,left,right;
	if(my_id != 0 && (my_id-1)/col == my_id/col){
		left = my_id-1;
    }else{
    	left = my_id+col-1;
    }
    if((my_id+1)/col == my_id/col){
    	right = my_id+1;
    }else{
    	right = my_id-col+1;
    }
    if((my_id-col)>=0){
    	up = my_id-col;
    }else{
    	up = my_id+(row-1)*col;
    }
    if((my_id+col)<num_procs){
    	down = my_id+col;
    }else{
    	down = my_id-(row-1)*col;
    }

	// RDMA操作的四个数据包
	struct glex_rdma_req rdmaReq3 = {
		.rmt_ep_addr = remote_ep_addr[down],
		.local_mh = mh[3],
		.local_offset = 0,
		.len = sizeof(double)*buffsize,
		.rmt_mh = remote_mem_addr[3],
		.rmt_offset = 0,
		.type = GLEX_RDMA_TYPE_PUT,
//		.local_evt = ,
		.rmt_evt = remote_event3,
		.rmt_key = 13,			// ep_attr初始化时设置的
//		.coll_counter = ,
//		.coll_set_num = ,
		.flag = GLEX_FLAG_REMOTE_EVT,
		.next = NULL
	};
	struct glex_rdma_req rdmaReq2 = {
		.rmt_ep_addr = remote_ep_addr[up],
		.local_mh = mh[2],
		.local_offset = 0,
		.len = sizeof(double)*buffsize,
		.rmt_mh = remote_mem_addr[2],
		.rmt_offset = 0,
		.type = GLEX_RDMA_TYPE_PUT,
//		.local_evt = ,
		.rmt_evt = remote_event2,
		.rmt_key = 13,			// ep_attr初始化时设置的
//		.coll_counter = ,
//		.coll_set_num = ,
		.flag = GLEX_FLAG_REMOTE_EVT,
		.next = &rdmaReq3
	};
	struct glex_rdma_req rdmaReq1 = {
		.rmt_ep_addr = remote_ep_addr[right],
		.local_mh = mh[1],
		.local_offset = 0,
		.len = sizeof(double)*buffsize,
		.rmt_mh = remote_mem_addr[1],
		.rmt_offset = 0,
		.type = GLEX_RDMA_TYPE_PUT,
//		.local_evt = ,
		.rmt_evt = remote_event1,
		.rmt_key = 13,			// ep_attr初始化时设置的
//		.coll_counter = ,
//		.coll_set_num = ,
		.flag = GLEX_FLAG_REMOTE_EVT,
		.next = &rdmaReq2
	};
	struct glex_rdma_req rdmaReq0 = {
		.rmt_ep_addr = remote_ep_addr[left],
		.local_mh = mh[0],
		.local_offset = 0,
		.len = sizeof(double)*buffsize,
		.rmt_mh = remote_mem_addr[0],
		.rmt_offset = 0,
		.type = GLEX_RDMA_TYPE_PUT,
//		.local_evt = ,
		.rmt_evt = remote_event0,
		.rmt_key = 13,			// ep_attr初始化时设置的
//		.coll_counter = ,
//		.coll_set_num = ,
		.flag = GLEX_FLAG_REMOTE_EVT,
		.next = &rdmaReq1
	};

	printf("process %d——left:%d,right:%d,up:%d,down:%d\n",my_id,remote_ep_addr[left].s.nic_id,remote_ep_addr[right].s.nic_id,remote_ep_addr[up].s.nic_id,remote_ep_addr[down].s.nic_id);

	/* 开始glex-RDMA版二维影响区交换 */
	MPI_Barrier(MPI_COMM_WORLD); 
    inittime = MPI_Wtime();

	// 发送RDMA PUT通信请求
	ret = glex_rdma(ep, &rdmaReq0, NULL);
	TEST_RetSuccess(ret, "非阻塞RDMA写失败！");
    
    glex_event_t *event = (glex_event_t *)malloc(10*sizeof(glex_event_t));
    for(fx=0; fx<4; fx++){
		ret = glex_probe_first_event(ep, -1, &event);
		TEST_RetSuccess(ret, "被写端点未接收到触发事件！");
		printf("process %d_%d —— cookie_0:%d, cookie_1:%d\n", my_id, fx, event[0].cookie_0, event[0].cookie_1);
    	ret = glex_discard_probed_event(ep);
		TEST_RetSuccess(ret, "清除event队列异常！");
    }

	MPI_Barrier(MPI_COMM_WORLD); 
	recvtime = MPI_Wtime();
    totaltime = recvtime - inittime;
	printf("process %d —— 二维影响区交换已经完成，用时 %.4lf ms\n", my_id, totaltime*1000);
	

	/* 其他工作… */



	MPI_Finalize();

	/* 释放内存 */
	for(fx=0; fx<4; fx++){
		ret = glex_deregister_mem(ep, mh[fx]);
		ret = glex_deregister_mem(ep, remote_mem_addr[fx]);
	}
	free(sendbuff);
	free(recvbuff);

	/* 释放端点资源 */
	ret = glex_destroy_ep(ep);

	/* 关闭GLEX设备 */
	ret = glex_close_device(dev);

	return 0;
}
