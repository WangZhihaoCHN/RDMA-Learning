#if HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "./infiniband/verbs.h"

#include "mpi.h"

#define RDMA_WRID 3

static int die(const char *reason){
	fprintf(stderr, "Err: %s - %s\n ", strerror(errno), reason);
	exit(EXIT_FAILURE);
	return -1;
}

// 如果x不是0，则打印错误
#define TEST_NZ(x,y) do { if ((x)) die(y); } while (0)

// 如果x是0，则打印错误
#define TEST_Z(x,y) do { if (!(x)) die(y); } while (0)

// 如果x为负，则打印错误
#define TEST_N(x,y) do { if ((x)<0) die(y); } while (0)

static int page_size;
static int sl = 1;
static pid_t pid;

struct app_context{
	struct ibv_context 		*context;
	struct ibv_pd      		*pd;
	struct ibv_mr      		*mr;
	struct ibv_cq      		*rcq;
	struct ibv_cq      		*scq;
	struct ibv_qp      		*qp;
	struct ibv_comp_channel *ch;
	void               		*buf;
	unsigned            	size;
	int                 	tx_depth;
	struct ibv_sge      	sge_list;
	struct ibv_send_wr  	wr;
};

struct ib_connection {
    int             	lid;
    int            	 	qpn;
    int             	psn;
	unsigned 			rkey;
	unsigned long long 	vaddr;
};

struct app_data {
	int							ib_port;
	unsigned            		size;
	int                 		tx_depth;
	char						*servername;
	struct ib_connection		local_connection;
	struct ib_connection 		*remote_connection;
	struct ibv_device			*ib_dev;

};

/**
 * 	  函数：qp_change_state_init
 *	  作用：该函数用于改变队列对状态来进行初始化。
 **/
static int qp_change_state_init(struct ibv_qp *qp, struct app_data *data){
	
	struct ibv_qp_attr *attr;

    attr =  malloc(sizeof *attr);
    memset(attr, 0, sizeof *attr);

    attr->qp_state        	= IBV_QPS_INIT;
    attr->pkey_index      	= 0;
    attr->port_num        	= data->ib_port;
    attr->qp_access_flags	= IBV_ACCESS_REMOTE_WRITE;

    TEST_NZ(ibv_modify_qp(qp, attr,
                            IBV_QP_STATE        |
                            IBV_QP_PKEY_INDEX   |
                            IBV_QP_PORT         |
                            IBV_QP_ACCESS_FLAGS),
            "不能够通过修改QP来进行INIT, ibv_modify_qp调用失败。");

	return 0;
}

/**
 * 	  函数：init_ctx
 *	  作用：该函数用于初始化Infiniband Context。它创建了保护域(ProtectionDomain)、内存域(MemoryRegion)、
 * 完成通道(CompletionChannel)、完成队列(CompletionQueues)以及队列对(QueuePair)。
 **/
static struct app_context *init_ctx(struct app_data *data)
{
	struct app_context *ctx;
	struct ibv_device *ib_dev;

	ctx = malloc(sizeof *ctx);
	memset(ctx, 0, sizeof *ctx);
	
	ctx->size = data->size;
	ctx->tx_depth = data->tx_depth;
	
	TEST_NZ(posix_memalign(&(ctx->buf), page_size, ctx->size * 2),
				"无法分配工作缓冲区ctx-> buf");

	memset(ctx->buf, 0, ctx->size * 2);

	struct ibv_device **dev_list;

	TEST_Z(dev_list = ibv_get_device_list(NULL),
            "无可用的IB设备。get_device_list返回NULL。");
	
    TEST_Z(data->ib_dev = dev_list[0],
            "无法分配IB设备。或许dev_list数组为空。");

	TEST_Z(ctx->context = ibv_open_device(data->ib_dev),
			"不能够创建context, ibv_open_device调用失败。");
	
	TEST_Z(ctx->pd = ibv_alloc_pd(ctx->context),
			"无法申请保护域(ProtectionDomain), ibv_alloc_pd调用失败。");

	/* 
     * 我们真的不想要IBV_ACCESS_LOCAL_WRITE，但是IB规范要求：
     * 在一块内存区域尚未分配为本地可写时，不允许使用远程写入或远程原子操作。
	 */
	TEST_Z(ctx->mr = ibv_reg_mr(ctx->pd, ctx->buf, ctx->size * 2, 
				IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE),
			"无法分配mr, ibv_reg_mr。请查看您是否有root权限?");
	
	TEST_Z(ctx->ch = ibv_create_comp_channel(ctx->context),
			"无法创建完成通道(CompletionChannel), ibv_create_comp_channel调用失败。");

	TEST_Z(ctx->rcq = ibv_create_cq(ctx->context, 1, NULL, NULL, 0),
			"无法创建接收完成队列(ReceiveCompletionQueues), ibv_create_cq调用失败。");	

	TEST_Z(ctx->scq = ibv_create_cq(ctx->context,ctx->tx_depth, ctx, ctx->ch, 0),
			"无法创建发送完成队列(SendCompletionQueues), ibv_create_cq调用失败。");

	struct ibv_qp_init_attr qp_init_attr = {
		.send_cq = ctx->scq,
		.recv_cq = ctx->rcq,
		.qp_type = IBV_QPT_RC,
		.cap = {
			.max_send_wr = ctx->tx_depth,
			.max_recv_wr = 1,
			.max_send_sge = 1,
			.max_recv_sge = 1,
			.max_inline_data = 0
		}
	};

	TEST_Z(ctx->qp = ibv_create_qp(ctx->pd, &qp_init_attr),
			"无法创建队列对(QueuePair), ibv_create_qp调用失败。");	
	
	qp_change_state_init(ctx->qp, data);
	
	return ctx;
}

static void destroy_ctx(struct app_context *ctx){

	TEST_NZ(ibv_destroy_qp(ctx->qp),
		"Could not destroy queue pair, ibv_destroy_qp");

	TEST_NZ(ibv_destroy_cq(ctx->scq),
		"Could not destroy send completion queue, ibv_destroy_cq");

	TEST_NZ(ibv_destroy_cq(ctx->rcq),
		"Coud not destroy receive completion queue, ibv_destroy_cq");

	TEST_NZ(ibv_destroy_comp_channel(ctx->ch),
		"Could not destory completion channel, ibv_destroy_comp_channel");

	TEST_NZ(ibv_dereg_mr(ctx->mr),
		"Could not de-register memory region, ibv_dereg_mr");

	TEST_NZ(ibv_dealloc_pd(ctx->pd),
		"Could not deallocate protection domain, ibv_dealloc_pd");	

	free(ctx->buf);
	free(ctx);
	
}

/**
 * 	  函数：set_local_ib_connection
 *    设置IB连接所需的所有相关属性值。然后通过MPI发送给对等点。
 * 通过IB交换数据所需的信息有：
 * lid   - 本地标识符，由子网管理器分配给末端节点的16位地址
 * qpn   - 队列对号，标识通道适配器（HCA）中的qpn
 * psn   - 数据包序列号，用于验证包裹的正确递送顺序（类似于ACK）
 * rkey  - Remote Key与'vaddr'一起标识并授予对内存区域的访问权限
 * vaddr - 虚拟地址，可以稍后写入的内存地址
 **/
static void set_local_ib_connection(struct app_context *ctx, struct app_data *data){

	// First get local lid
	struct ibv_port_attr attr;
	TEST_NZ(ibv_query_port(ctx->context,data->ib_port,&attr),
		"Could not get port attributes, ibv_query_port");

	data->local_connection.lid = attr.lid;
	data->local_connection.qpn = ctx->qp->qp_num;
	data->local_connection.psn = lrand48() & 0xffffff;
	data->local_connection.rkey = ctx->mr->rkey;
	data->local_connection.vaddr = (uintptr_t)(ctx->buf) + ctx->size;
}

static void print_ib_connection(char *conn_name, struct ib_connection *conn){
	printf("%s: LID %#04x, QPN %#06x, PSN %#06x RKey %#08x VAddr %#016Lx\n", 
			conn_name, conn->lid, conn->qpn, conn->psn, conn->rkey, conn->vaddr);
}

/*
 *  qp_change_state_rtr
 * **********************
 *  Changes Queue Pair status to RTR (Ready to receive)
 */
static int qp_change_state_rtr(struct ibv_qp *qp, struct app_data *data){
	
	struct ibv_qp_attr *attr;

    attr =  malloc(sizeof *attr);
    memset(attr, 0, sizeof *attr);

    attr->qp_state 				=IBV_QPS_INIT;
    attr->pkey_index 			=0;
    attr->port_num 				=data->ib_port;
    attr->qp_access_flags 		=0;

    TEST_NZ(ibv_modify_qp(qp, attr,
                IBV_QP_STATE      |
		  		IBV_QP_PKEY_INDEX |
		  		IBV_QP_PORT       |
		  		IBV_QP_ACCESS_FLAGS),
        "Could not modify QP to INIT state");

    memset(attr, 0, sizeof *attr);

    attr->qp_state              = IBV_QPS_RTR;
    attr->path_mtu              = IBV_MTU_2048;
    attr->dest_qp_num           = data->remote_connection->qpn;
    attr->rq_psn                = data->remote_connection->psn;
    attr->max_dest_rd_atomic    = 1;
    attr->min_rnr_timer         = 12;
    attr->ah_attr.is_global     = 0;
    attr->ah_attr.dlid          = data->remote_connection->lid;
    attr->ah_attr.sl            = sl;
    attr->ah_attr.src_path_bits = 0;
    attr->ah_attr.port_num      = data->ib_port;

    TEST_NZ(ibv_modify_qp(qp, attr,
                IBV_QP_STATE                |
                IBV_QP_AV                   |
                IBV_QP_PATH_MTU             |
                IBV_QP_DEST_QPN             |
                IBV_QP_RQ_PSN               |
                IBV_QP_MAX_DEST_RD_ATOMIC   |
                IBV_QP_MIN_RNR_TIMER),
        "Could not modify QP to RTR state");

	free(attr);
	
	return 0;
}

/*
 *  qp_change_state_rts
 * **********************
 *  Changes Queue Pair status to RTS (Ready to send)
 *	QP status has to be RTR before changing it to RTS
 */
static int qp_change_state_rts(struct ibv_qp *qp, struct app_data *data){

	// first the qp state has to be changed to rtr
	qp_change_state_rtr(qp, data);
	
	struct ibv_qp_attr *attr;

    attr =  malloc(sizeof *attr);
    memset(attr, 0, sizeof *attr);

	attr->qp_state              = IBV_QPS_RTS;
    attr->sq_psn                = data->local_connection.psn;
    attr->timeout               = 14;
    attr->retry_cnt             = 7;
    attr->rnr_retry             = 7;    /* infinite retry */
    attr->max_rd_atomic         = 1;

    TEST_NZ(ibv_modify_qp(qp, attr,
                IBV_QP_STATE            |
                IBV_QP_TIMEOUT          |
                IBV_QP_RETRY_CNT        |
                IBV_QP_RNR_RETRY        |
                IBV_QP_SQ_PSN           |
                IBV_QP_MAX_QP_RD_ATOMIC),
        "Could not modify QP to RTS state");

    free(attr);

    return 0;
}

/*
 *  rdma_write
 * **********************
 *	Writes 'ctx-buf' into buffer of peer
 */
static void rdma_write(struct app_context *ctx, struct app_data *data){
	
	memset(&(ctx->sge_list), 0, sizeof(ctx->sge_list));
	ctx->sge_list.addr      = (uintptr_t)ctx->buf;
   	ctx->sge_list.length    = ctx->size;
   	ctx->sge_list.lkey      = ctx->mr->lkey;

   	memset(&(ctx->wr), 0, sizeof(ctx->wr));
  	ctx->wr.wr.rdma.remote_addr = data->remote_connection->vaddr;
    ctx->wr.wr.rdma.rkey        = data->remote_connection->rkey;
    ctx->wr.sg_list     = &(ctx->sge_list);
    ctx->wr.num_sge     = 1;
    ctx->wr.opcode      = IBV_WR_RDMA_WRITE;

    printf("接收节点信息: rkey %x, vaddr %016Lx \n", ctx->wr.wr.rdma.rkey, ctx->wr.wr.rdma.remote_addr);
    printf("要发送的字符串为 %s \n", ctx->sge_list.addr);
    
    struct ibv_send_wr *bad_wr;

    int ret = ibv_post_send(ctx->qp,&ctx->wr,&bad_wr);
    if(ret != 0){
    	printf("ibv_post_send错误代码为： %d \n", ret);
    	printf("%d表示——On failure and no change will be done to the QP and bad_wr points to the SR that failed to be posted \n", errno);
    	printf("%d表示——Invalid value provided in wr \n", EINVAL);
    	printf("%d表示——Send Queue is full or not enough resources to complete this operation \n", ENOMEM);
    	printf("%d表示——Invalid value provided in qp \n", EFAULT);
    }

    //TEST_NZ(ibv_post_send(ctx->qp,&ctx->wr,&bad_wr),
    //    "ibv_post_send failed. This is bad mkay");
}	

static void rdma_receive(struct app_context *ctx, struct app_data *data){
	
	ctx->sge_list.addr      = (uintptr_t)ctx->buf;
   	ctx->sge_list.length    = ctx->size;
   	ctx->sge_list.lkey      = ctx->mr->lkey;

	printf("Receiver TEST——Printing local buffer: %s\n" ,ctx->sge_list.addr);

    struct ibv_recv_wr wr = {
		.next = NULL,
		.sg_list = &(ctx->sge_list),
		.num_sge = 1
	};

    struct ibv_recv_wr *bad_wr;

    printf("接收节点信息: lkey %x, vaddr %016Lx \n", data->remote_connection->vaddr);
    
    TEST_NZ(ibv_post_recv(ctx->qp,&wr,&bad_wr),
        "ibv_post_recv failed. This is bad mkay");
}	


int main(int argc, char *argv[])
{
	/* 初始化MPI环境 */
	int num_procs,my_id;		//进程总数、进程ID
	MPI_Status status;			//status对象(Status)
	MPI_Request handle;			//MPI请求(request)

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&num_procs);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_id);

	TEST_Z(num_procs == 2,
          "本程序需要且仅需要两个进程(发送和接收)。");

	printf("本程序共有 %d 个进程！\n", num_procs);

	struct app_context 		*ctx = NULL;
	char                    *ib_devname = NULL;
	int                   	iters = 1000;
	int                  	scnt, ccnt;
	int                   	duplex = 0;
	struct ibv_qp			*qp;

	struct app_data	 	 data = {
		.ib_port    		= 1,
		.size       		= 256,
		.tx_depth   		= 100,
		.servername 		= NULL,
		.remote_connection 	= NULL,
		.ib_dev     		= NULL
		
	};

	if(my_id == 0){
		data.servername = "Sender";
	}else{
		data.servername = "Receiver";
	}
	pid = getpid();

	// 打印APP参数。这基本上来自rdma_bw应用程序。 它们大多数都没有使用atm。
	printf("%s : | PID=%d | ib_port=%d | size=%d | tx_depth=%d | sl=%d |\n",
		data.servername, pid, data.ib_port, data.size, data.tx_depth, sl);

	// 之后需要为psn创建随机数
	srand48(pid * time(NULL));
	
	page_size = sysconf(_SC_PAGESIZE);
	
	TEST_Z(ctx = init_ctx(&data),
          "无法创建ctx, init_ctx调用失败。");
	printf("RDMA初始化完成。\n");

	set_local_ib_connection(ctx, &data);

	/* 利用MPI交换发送节点和接收节点的IB信息。 */
	char msg[sizeof "0000:000000:000000:00000000:0000000000000000"];
	char remoteMsg[sizeof "0000:000000:000000:00000000:0000000000000000"];
	int parsed;

	struct ib_connection *local = &(data.local_connection);

	sprintf(msg, "%04x:%06x:%06x:%08x:%016Lx", 
				local->lid, local->qpn, local->psn, local->rkey, local->vaddr);
	
	if(my_id == 0){
		/* 发送节点将自身ib信息发送个接收节点 */
		MPI_Isend(msg, strlen(msg), MPI_CHAR, 1, 0, MPI_COMM_WORLD, &handle);
		MPI_Recv(remoteMsg, strlen(msg), MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status);
		
		/* 为data中的远程信息数据结构申请内存 */
		TEST_Z(data.remote_connection = malloc(sizeof(struct ib_connection)),
			"Process 0 could not allocate memory for remote_connection connection");
	}else{
		/* 接收节点将自身ib信息发送给发送节点 */
		MPI_Isend(msg, strlen(msg), MPI_CHAR, 0, 0, MPI_COMM_WORLD, &handle);
		MPI_Recv(remoteMsg, strlen(msg), MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
		
		/* 将接收的数据存储于data中的远程连接中 */
		TEST_Z(data.remote_connection = malloc(sizeof(struct ib_connection)),
			"Process 1 could not allocate memory for remote_connection connection");
	}
	/* 将接收的数据存储于data中的远程连接中指针中 */
	struct ib_connection *remote = data.remote_connection;
		parsed = sscanf(remoteMsg, "%x:%x:%x:%x:%Lx", 
						&(remote->lid), &(remote->qpn), &(remote->psn), &(remote->rkey), &(remote->vaddr));
	
	/* 验证接收过程和存储过程是否正常*/
	if(parsed != 5){
		fprintf(stderr, "Could not parse message from peer");
		free(data.remote_connection);
	}

	// 打印IB-连接详细信息,转换队列对状态
	if(my_id == 0){
		print_ib_connection("Sender's Local  Connection", &data.local_connection);
		print_ib_connection("Sender's Remote Connection", data.remote_connection);	
		qp_change_state_rtr(ctx->qp, &data);
	}else{
		print_ib_connection("Receiver's Local  Connection", &data.local_connection);
		print_ib_connection("Receiver's Remote Connection", data.remote_connection);
		qp_change_state_rts(ctx->qp, &data);
	}

	if(my_id == 0){
		/* Server——RDMA写 */
		printf("Server. Writing to Client-Buffer (RDMA-WRITE)\n");

		// 现在，要写入客户端缓冲区的消息可以在这里编辑
		char *chPtr = ctx->buf;
		strcpy(chPtr,"Hello man, This is my first time to use RDMA!\n");

		rdma_write(ctx, &data);
		
	}else{
		/* Client - Read local buffer */
		printf("Client. Reading Local-Buffer (Buffer that was registered with MR)\n");
		
		char *chPtr = (char *)data.local_connection.vaddr;
		memset(chPtr, 0, sizeof *chPtr);
		printf("Receiver: 读取的地址为 %016Lx\n", data.local_connection.vaddr);

		while(1){
			sleep(10);
			printf("Printing local buffer: %s\n" ,chPtr);
		}
	
	}

	printf("关闭IB context\n");
	destroy_ctx(ctx);

	//两个进程，进程1检查RDMA devices
	if(my_id == 0){
		printf("Process 0 run over!\n");
	}else{
		printf("Process 1 run over!\n");
	}
	
	MPI_Finalize();
	return 0;
}
