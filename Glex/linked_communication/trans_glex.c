#include "mpi.h"
#include "glex.h"
#include <time.h>
#include <memory.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>


void glex_rdma_recv(int rank,int flag,int r1,int r2,glex_ep_handle_t ep);
void glex_rdma_put_counter(int r,glex_ep_handle_t ep,glex_ep_addr_t remote_ep_addr,glex_mem_handle_t mh,glex_mem_handle_t remote_mem_addr,glex_ret_t ret,int offset);
void glex_rdma_put(int r,glex_ep_handle_t ep,glex_ep_addr_t remote_ep_addr,glex_mem_handle_t mh,glex_mem_handle_t remote_mem_addr,glex_ret_t ret,int offset);

void  TEST_RetSuccess(glex_ret_t glex_ret,char* str){
      if(strcmp(glex_error_str(glex_ret),"successful") != 0){
          printf("Err :%s--%s\n",glex_error_str(glex_ret),str);
          exit(EXIT_FAILURE);
          return;
      }
 }

int main(int argc,char **argv){

    //rdma能传输的最大长度为128M
    const int maxn = 209715200;
 
    //时间结构体
    struct timespec start,end;
    //对开始时间和结束时间做初始化操作
    memset(&start,0,sizeof(start));
    memset(&end,0,sizeof(end));


    //获得传输的消息大小、矩阵的长和宽
    int message_size,m,n;
    sscanf(argv[argc - 1],"%d",&message_size);
    sscanf(argv[argc - 2],"%d",&n);
    sscanf(argv[argc - 3],"%d",&m);

    //建立一个输运扫描矩阵
    int trans_rank[1000][1000];

    int i,j,num = 0;
    for (i = 0;i < m;i++){
        for (j = 0;j < n;j++){
            trans_rank[i][j] = num++;
        }
    }

    //进程号
    int rank;
    //所有glex函数都会返回这个值
    glex_ret_t ret;
    glex_ep_handle_t ep;   //存放创建成功的端点的标识符
    /* 将接收的数据存储于remote_ep_addr */
    // 右节点的远程端点地址
    glex_ep_addr_t right_remote_ep_addr;
    // 下节点的远程端点地址
    glex_ep_addr_t down_remote_ep_addr;
    // 本地端点地址
    glex_ep_addr_t ep_addr;
    //存储远程端点内存标识
    //右节点的端点内存标识
    glex_mem_handle_t right_remote_mem_addr;
    //下节点的端点内存标识
    glex_mem_handle_t down_remote_mem_addr;
    //本地内存标识
    glex_mem_handle_t mh;
    //进程号，开启的进程数
    int size,buf[1];
    MPI_Status status;

    //初始化MPI环境
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    
    //获取开始时间
    clock_gettime(CLOCK_REALTIME,&start);


    /**** 配置GLEX环境****/
 	
 	glex_device_handle_t dev;


 	/* 打开GLEX-0号设备(经测试，每台机器上有两个GLEX设备，1号不能被打开)  */
 	ret = glex_open_device(0,&dev);   //dev用于存储被打开设备的标识符
 	TEST_RetSuccess(ret,"0号设备通信接口打开失败！");


 	/* 在通信接口上创建一个端点，作为虚拟化硬件通信资源的软件实体 */
 	
 	struct glex_ep_attr ep_attr = {
 		.type = GLEX_EP_TYPE_FAST,
 		.mpq_type = GLEX_MPQ_TYPE_NORMAL,
 		.eq_type = GLEX_EQ_TYPE_HIGH_CAPACITY,
 		.num = GLEX_ANY_EP_NUM,
 		.key = 13,
        //.eq_capacity = 300
 	};
    struct glex_ep_attr ep_temp_attr;

 	ret = glex_create_ep(dev,&ep_attr,&ep);

    //glex_query_ep(ep,&ep_temp_attr);
    //printf("ep : %d\n",ep_temp_attr.dq_capacity);

 	/* 获得指定端点的端点地址 */
 	ret = glex_get_ep_addr(ep,&ep_addr);


 	/* 注册内存，锁内存并建立映射关系 */
    
    char* mem_addr = (char*)malloc(sizeof(char) * maxn);
 
    ret = glex_register_mem(ep,mem_addr,sizeof(char)*maxn,GLEX_MEM_READ|GLEX_MEM_WRITE,&mh);
    TEST_RetSuccess(ret,"内存锁定失败！");


 	/**** 0号进程将自己的端点地址发送到其余所有的进程，以便其余进程能将数据传过来(glex_ep_addr_t) ****/
 	/* 定义本地端点地址和远程端点地址的char数组，便于MPI发送 */
 	char msg[sizeof("0000:0000:00000000:0000000000000000")];
 	char remoteMsg[sizeof("0000:0000:00000000:0000000000000000")];
 	int parsed;   //用于验证接收后参数数量是否正确
 	sprintf(msg,"%04x:%04x:%08x:%016Lx",ep_addr.s.ep_num,ep_addr.s.resv,ep_addr.s.nic_id,ep_addr.v);

 	/* 利用MPI，交换发送、接收端点的地址(glex_ep_addr_t) */
    /* 如果存在左节点和右节点，需要将自己的地址发送给左节点和右节点,并且相应的左节点和上节点需要接收信息 */
 	/* 将接收的数据存储于remote_ep_addr */

    //首先获得当前节点所在的矩阵的行与列
    for (i = 0;i < m;i++){
        int b = 0;
        for (j = 0;j < n;j++){
            if (trans_rank[i][j] == rank){
                b = 1;
                break;
            }
        }
        if (b) break;
    }
    
    //printf("rank : %d, i=%d, j=%d\n",rank,i,j);
    //MPI_Finalize();
    //return 0;

    //printf("rank : %d , 当前节点的端点地址 : %s\n",rank,msg);
    //判断当前节点是否有左节点
    if (j - 1 >= 0) {
        //printf("rank : %d, send : %d\n",rank,trans_rank[i][j - 1]);
        MPI_Send(msg,sizeof(msg),MPI_CHAR,trans_rank[i][j - 1],0,MPI_COMM_WORLD);
    }
    //判断当前节点是否有上节点
    if (i - 1 >= 0) {
        //printf("rank : %d, send : %d\n",rank,trans_rank[i - 1][j]);
        MPI_Send(msg,sizeof(msg),MPI_CHAR,trans_rank[i - 1][j],0,MPI_COMM_WORLD);
    }
    //判断当前节点是否有右节点
    if (j + 1 < n){
        //printf("rank : %d, recv : %d\n",rank,trans_rank[i][j + 1]);
        MPI_Recv(remoteMsg,sizeof(msg),MPI_CHAR,trans_rank[i][j + 1],0,MPI_COMM_WORLD,&status);
        parsed = sscanf(remoteMsg,"%x:%x:%x:%Lx",&(right_remote_ep_addr.s.ep_num),&(right_remote_ep_addr.s.resv),&(right_remote_ep_addr.s.nic_id),&(right_remote_ep_addr.v));
        if (parsed != 4){
            fprintf(stderr,"%d进程接收右节点端点地址信息失败\n",rank);
        }
        //printf("rank : %d , 右节点的端点地址: %s\n",rank,remoteMsg);
    }
    //判断当前节点是否有下节点
    if (i + 1 < m) {
        //printf("rank : %d, recv : %d\n",rank,trans_rank[i + 1][j]);
        MPI_Recv(remoteMsg,sizeof(msg),MPI_CHAR,trans_rank[i + 1][j],0,MPI_COMM_WORLD,&status);
        parsed = sscanf(remoteMsg,"%x:%x:%x:%Lx",&(down_remote_ep_addr.s.ep_num),&(down_remote_ep_addr.s.resv),&(down_remote_ep_addr.s.nic_id),&(down_remote_ep_addr.v));
        if (parsed != 4) {
            fprintf(stderr,"%d进程接收下节点端点地址信息失败\n",rank);
        }
        //printf("rank : %d , 下节点的端点地址 : %s\n",rank,remoteMsg);
    }
    
    //printf("端点地址交换成功\n");
    //MPI_Finalize();
    //return 0;

 	//printf("本地端点NIC ID : %d , 端点序号 : %d , 远程端点NIC ID: %d\n",nicID,EPNum,remote_ep_addr.s.nic_id);

 	/* 定义本地端点地址和远程端点地址的char数组(交换内存标识信息) */
 	sprintf(msg,"%04x:%04x:%08x:%016Lx",mh.s.mmt_index,mh.s.att_base_off,mh.s.att_index,mh.v);

 	/* 利用MPI，交换发送，接收端点的地址(glex_mem_handle_t) */
    
    //printf("rank : %d , 当前节点的内存地址 : %s\n",rank,msg);
    if (j - 1 >= 0) {
        MPI_Send(msg,sizeof(msg),MPI_CHAR,trans_rank[i][j - 1],1,MPI_COMM_WORLD);
    }
    if (i - 1 >= 0) {
        MPI_Send(msg,sizeof(msg),MPI_CHAR,trans_rank[i - 1][j],1,MPI_COMM_WORLD);
    }
    if (j + 1 < n) {
        MPI_Recv(remoteMsg,sizeof(msg),MPI_CHAR,trans_rank[i][j + 1],1,MPI_COMM_WORLD,&status);
        parsed = sscanf(remoteMsg,"%x:%x:%x:%Lx",&(right_remote_mem_addr.s.mmt_index),&(right_remote_mem_addr.s.att_base_off),&(right_remote_mem_addr.s.att_index),&(right_remote_mem_addr.v));
        if (parsed != 4) {
            fprintf(stderr,"%d进程接收右节点端点内存地址失败\n",rank);
        }
        //printf("rank : %d, 右节点的内存地址 : %s\n",rank,remoteMsg);
    }
    if (i + 1 < m) {
        MPI_Recv(remoteMsg,sizeof(msg),MPI_CHAR,trans_rank[i + 1][j],1,MPI_COMM_WORLD,&status);
        parsed = sscanf(remoteMsg,"%x:%x:%x:%Lx",&(down_remote_mem_addr.s.mmt_index),&(down_remote_mem_addr.s.att_base_off),&(down_remote_mem_addr.s.att_index),&(down_remote_mem_addr.v));
        if (parsed != 4) {
            fprintf(stderr,"%d进程接收下节点端点内存地址失败\n",rank);
        }
        //printf("rank : %d, 下节点的内存地址 : %s\n",rank,remoteMsg);
    }

    //printf("交换端点地址和内存标识成功\n");
    //MPI_Finalize();

    //return 0;

 	/* 利用GLEX，在本地端点和远程端点之间。进行RDMA操作 */
    // 
    //struct timespec start,end;
    //memset(&start,0,sizeof(start));
    //memset(&end,0,sizeof(end));
    //clock_gettime(CLOCK_REALTIME,&end);
    
    //printf("初始化的时间为 : %ld\n",end.tv_nsec - start.tv_nsec);
    char c[1];
    sprintf(c,"%d",rank);
    memset(mem_addr,c[0],message_size);
    
    double t_start = MPI_Wtime(),t_end = 0.0;

    int iterations = 0,skip = 0;

    if (message_size > 8192){
        iterations = 1000;
        skip = 10;
    }else{
        iterations = 10000;
        skip = 100;
    }

    int k;

    glex_event_t *event = (glex_event_t *)malloc(10 * sizeof(glex_event_t));
    MPI_Barrier(MPI_COMM_WORLD);
    for (k = 0;k < iterations + skip;k++){
      
      if (k == skip && rank == trans_rank[m - 1][n - 1]){
           t_start = MPI_Wtime();
      }
      
        //当前节点既没有左节点也没有上节点，设置自己的counter为0
        if (j - 1 < 0 && i - 1 < 0){
            
            //当前节点既有右节点也有下节点

            //触发的远程事件
            /*
            struct glex_event remote_event = {
                .cookie_0 = rank,
                .cookie_1 = rank + 1
            };
            */
            struct glex_rdma_req down_rdmaReq = {
                .rmt_ep_addr = down_remote_ep_addr,
                .local_mh = mh,
                .local_offset = 0,
                .len = message_size*2,
                .rmt_mh = down_remote_mem_addr,
                .rmt_offset = message_size,
                .type = GLEX_RDMA_TYPE_PUT,
                //.rmt_evt = remote_event,
                .rmt_key = 13,
                //.coll_counter = counter,
                .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL |  GLEX_FLAG_COLL_COUNTER,
                .next = NULL
            };
            struct glex_rdma_req right_rdmaReq = {
                .rmt_ep_addr = right_remote_ep_addr,
                .local_mh = mh,
                .local_offset = 0,
                .len = message_size,
                .rmt_mh = right_remote_mem_addr,
                .rmt_offset = message_size,
                .type = GLEX_RDMA_TYPE_PUT,
                //.rmt_evt = remote_event,
                .rmt_key = 13,
                .coll_counter = 0,
                .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START  | GLEX_FLAG_COLL_COUNTER,
                .next = &down_rdmaReq
            };

            //发送RDMA PUT通信请求
            ret = glex_rdma(ep,&right_rdmaReq,NULL);
            TEST_RetSuccess(ret,"非阻塞RDMA写失败！");

            // 轮询检查RDMA操作是否出现错误请求
            uint32_t num_er = 0;
            struct glex_err_req *er_list;
            ret = glex_poll_error_req(ep,&num_er,er_list);
            if(num_er != 0){
                printf("rank : %d, %s, 出错的个数为%d\n",rank,"glex_rdma_put_counter操作失败，num_er不为0.",num_er);
                printf("出错的原因为：%s\n",glex_error_req_status_str(er_list[0].status));
            }


                
        }
        //当前节点只有左节点,设置自己的counter为1
        if (j - 1 >= 0 && i - 1 < 0){
            //当前节点只有下节点
            if (i + 1 < m && j + 1 >= n){
                //触发的远程事件
                /*
                struct glex_event remote_event = {
                    .cookie_0 = rank,
                    .cookie_1 = rank + 1
                };
                */
                struct glex_rdma_req down_rdmaReq = {
                    .rmt_ep_addr = down_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = down_remote_mem_addr,
                    .rmt_offset = message_size * 2,
                    .type = GLEX_RDMA_TYPE_PUT,
                    //.rmt_evt = remote_event,
                    .rmt_key = 13,
                    .coll_counter = 1,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START |  GLEX_FLAG_COLL_COUNTER,
                    .next = NULL
                };
                //发送RDMA PUT通信请求
                ret = glex_rdma(ep,&down_rdmaReq,NULL);
                TEST_RetSuccess(ret,"非阻塞RDMA写失败！");

                // 轮询检查RDMA操作是否出现错误请求
                uint32_t num_er = 0;
                struct glex_err_req *er_list;
                ret = glex_poll_error_req(ep,&num_er,er_list);
                if(num_er != 0){
                    printf("rank : %d, %s, 出错的个数为%d\n",rank,"glex_rdma_put_counter操作失败，num_er不为0.",num_er);
                    printf("出错的原因为：%s\n",glex_error_req_status_str(er_list[0].status));
                }

            }
            //当前节点既有右节点也有下节点
            if (i + 1 < m && j + 1 < n){
                //触发的远程事件
                /*
                struct glex_event remote_event = {
                    .cookie_0 = rank,
                    .cookie_1 = rank + 1
                };
                */
                struct glex_rdma_req down_rdmaReq = {
                    .rmt_ep_addr = down_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = down_remote_mem_addr,
                    .rmt_offset = message_size * 2,
                    .type = GLEX_RDMA_TYPE_PUT,
                    //.rmt_evt = remote_event,
                    .rmt_key = 13,
                    //.coll_counter = 1,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER,
                    .next = NULL
                };
                struct glex_rdma_req right_rdmaReq = {
                    .rmt_ep_addr = right_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = right_remote_mem_addr,
                    .rmt_offset = message_size,
                    .type = GLEX_RDMA_TYPE_PUT,
                    //.rmt_evt = remote_event,
                    .rmt_key = 13,
                    .coll_counter = 1,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER,
                    .next = &down_rdmaReq
                };
                //发送RDMA PUT通信请求
                ret = glex_rdma(ep,&right_rdmaReq,NULL);
                TEST_RetSuccess(ret,"非阻塞RDMA写失败！");

                // 轮询检查RDMA操作是否出现错误请求
                uint32_t num_er = 0;
                struct glex_err_req *er_list;
                ret = glex_poll_error_req(ep,&num_er,er_list);
                if(num_er != 0){
                    printf("rank : %d, %s, 出错的个数为%d\n",rank,"glex_rdma_put_counter操作失败，num_er不为0.",num_er);
                    printf("出错的原因为：%s\n",glex_error_req_status_str(er_list[0].status));
                }
            }

            //glex_rdma_recv(rank,1,trans_rank[i][j - 1],0,ep);
            //printf("接收节点：%d, 接收后，buffer内容是，%s\n",rank, mem_addr);
        }
        //当前节点只有上节点，设置自己的counter为1
        if (i - 1 >= 0 && j - 1 < 0){
            //当前节点只有右节点
            if (j + 1 < n && i + 1 >= m){
                //触发的远程事件
                /*
                struct glex_event remote_event = {
                    .cookie_0 = rank,
                    .cookie_1 = rank + 1
                };
                */
                struct glex_rdma_req right_rdmaReq = {
                    .rmt_ep_addr = right_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = right_remote_mem_addr,
                    .rmt_offset = message_size,
                    .type = GLEX_RDMA_TYPE_PUT,
                    //.rmt_evt = remote_event,
                    .rmt_key = 13,
                    .coll_counter = 1,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER,
                    .next = NULL
                };
                //发送RDMA PUT通信请求
                ret = glex_rdma(ep,&right_rdmaReq,NULL);
                TEST_RetSuccess(ret,"非阻塞RDMA写失败！");

                // 轮询检查RDMA操作是否出现错误请求
                uint32_t num_er = 0;
                struct glex_err_req *er_list;
                ret = glex_poll_error_req(ep,&num_er,er_list);
                if(num_er != 0){
                    printf("rank : %d, %s, 出错的个数为%d\n",rank,"glex_rdma_put_counter操作失败，num_er不为0.",num_er);
                    printf("出错的原因为：%s\n",glex_error_req_status_str(er_list[0].status));
                }
            }
            //当前节点既有右节点也有下节点
            if (i + 1 < m && j + 1 < n){
                //触发的远程事件
                /*
                struct glex_event remote_event = {
                    .cookie_0 = rank,
                    .cookie_1 = rank + 1
                };
                */
                struct glex_rdma_req down_rdmaReq = {
                    .rmt_ep_addr = down_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = down_remote_mem_addr,
                    .rmt_offset = message_size,
                    .type = GLEX_RDMA_TYPE_PUT,
                    //.rmt_evt = remote_event,
                    .rmt_key = 13,
                    //.coll_counter = 1,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER,
                    .next = NULL
                };
                struct glex_rdma_req right_rdmaReq = {
                    .rmt_ep_addr = right_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = right_remote_mem_addr,
                    .rmt_offset = message_size,
                    .type = GLEX_RDMA_TYPE_PUT,
                    //.rmt_evt = remote_event,
                    .rmt_key = 13,
                    .coll_counter = 1,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER,
                    .next = &down_rdmaReq
                };
                //发送RDMA PUT通信请求
                ret = glex_rdma(ep,&right_rdmaReq,NULL);
                TEST_RetSuccess(ret,"非阻塞RDMA写失败！");

                // 轮询检查RDMA操作是否出现错误请求
                uint32_t num_er = 0;
                struct glex_err_req *er_list;
                ret = glex_poll_error_req(ep,&num_er,er_list);
                if(num_er != 0){
                    printf("rank : %d, %s, 出错的个数为%d\n",rank,"glex_rdma_put_counter操作失败，num_er不为0.",num_er);
                    printf("出错的原因为：%s\n",glex_error_req_status_str(er_list[0].status));
                }
            }
            //glex_rdma_recv(rank,1,trans_rank[i - 1][j],0,ep);
            //printf("接收节点：%d, 接收后，buffer内容是，%s\n",rank, mem_addr);
        }
        //当前节点既有左节点又有上节点，设置自己的counter为2
        if (i - 1 >= 0 && j - 1 >= 0){
            //当前节点既没有右节点也没有下节点，这是终点
           
            if (i + 1 >= m && j + 1 >= n){
                //当接收到相应的事件后显示内存中的内容
                //glex_event_t *event = (glex_event_t *)malloc(10 * sizeof(glex_event_t));
                int b1 = 0,b2 = 0;
                while(1){
                    if (b1 && b2) break;
                    glex_probe_first_event(ep,20,&event);
                    if (event[0].cookie_0 == trans_rank[i][j - 1] && event[0].cookie_1 == trans_rank[i][j - 1] + 1){
                        b1 = 1;
                    }
                    if (event[0].cookie_0 == trans_rank[i - 1][j] && event[0].cookie_1 == trans_rank[i - 1][j] + 1){
                        b2 = 1;
                    }
                    //printf("cookie_0 : %d, cookie_1 : %d\n",event[0].cookie_0,event[0].cookie_1);
                    //printf("b1 : %d , b2 : %d\n",b1,b2);
                    glex_discard_probed_event(ep);
                
                }
                
                //printf("接收节点：%d, 接收后，buffer内容是，%s\n",rank, mem_addr);
                //t_end = MPI_Wtime();
                //glex_rdma_recv(rank,2,trans_rank[i][j - 1],trans_rank[i - 1][j],ep);
                //printf("执行时间为 %lf\n", (t_end - t_start) * 1e6);

            }
            
            //当前节点只有右节点
            if (j + 1 < n && i + 1 >= m){
                //触发的远程事件
                struct glex_event remote_event = {
                    .cookie_0 = rank,
                    .cookie_1 = rank + 1
                };
                struct glex_rdma_req right_rdmaReq = {
                    .rmt_ep_addr = right_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = right_remote_mem_addr,
                    .rmt_offset = message_size,
                    .type = GLEX_RDMA_TYPE_PUT,
                    .rmt_evt = remote_event,
                    .rmt_key = 13,
                    .coll_counter = 2,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_REMOTE_EVT,
                    .next = NULL
                };
                //发送RDMA PUT通信请求
                ret = glex_rdma(ep,&right_rdmaReq,NULL);
                TEST_RetSuccess(ret,"非阻塞RDMA写失败！");

                // 轮询检查RDMA操作是否出现错误请求
                uint32_t num_er = 0;
                struct glex_err_req *er_list;
                ret = glex_poll_error_req(ep,&num_er,er_list);
                if(num_er != 0){
                    printf("rank : %d, %s, 出错的个数为%d\n",rank,"glex_rdma_put_counter操作失败，num_er不为0.",num_er);
                    printf("出错的原因为：%s\n",glex_error_req_status_str(er_list[0].status));
                }
            }
            //当前节点只有下节点
            if (i + 1 < m && j + 1 >= n){
                //触发的远程事件
                struct glex_event remote_event = {
                    .cookie_0 = rank,
                    .cookie_1 = rank + 1
                };
                struct glex_rdma_req down_rdmaReq = {
                    .rmt_ep_addr = down_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = down_remote_mem_addr,
                    .rmt_offset = message_size * 2,
                    .type = GLEX_RDMA_TYPE_PUT,
                    .rmt_evt = remote_event,
                    .rmt_key = 13,
                    .coll_counter = 2,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_REMOTE_EVT,
                    .next = NULL
                };
                
                //发送RDMA PUT通信请求
                ret = glex_rdma(ep,&down_rdmaReq,NULL);
                TEST_RetSuccess(ret,"非阻塞RDMA写失败！");

                // 轮询检查RDMA操作是否出现错误请求
                uint32_t num_er = 0;
                struct glex_err_req *er_list;
                ret = glex_poll_error_req(ep,&num_er,er_list);
                if(num_er != 0){
                    printf("rank : %d, %s, 出错的个数为%d\n",rank,"glex_rdma_put_counter操作失败，num_er不为0.",num_er);
                    printf("出错的原因为：%s\n",glex_error_req_status_str(er_list[0].status));
                }
            }
            //当前节点既有右节点也有下节点
            if (i + 1 < m && j + 1 < n){
                //触发的远程事件
                /*
                struct glex_event remote_event = {
                    .cookie_0 = rank,
                    .cookie_1 = rank + 1
                };
                */
                struct glex_rdma_req down_rdmaReq = {
                    .rmt_ep_addr = down_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = down_remote_mem_addr,
                    .rmt_offset = message_size * 2,
                    .type = GLEX_RDMA_TYPE_PUT,
                    //.rmt_evt = remote_event,
                    .rmt_key = 13,
                    //.coll_counter = 1,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER,
                    .next = NULL
                };
                struct glex_rdma_req right_rdmaReq = {
                    .rmt_ep_addr = right_remote_ep_addr,
                    .local_mh = mh,
                    .local_offset = 0,
                    .len = message_size,
                    .rmt_mh = right_remote_mem_addr,
                    .rmt_offset = message_size,
                    .type = GLEX_RDMA_TYPE_PUT,
                    //.rmt_evt = remote_event,
                    .rmt_key = 13,
                    .coll_counter = 2,
                    .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER,
                    .next = &down_rdmaReq
                };
                //发送RDMA PUT通信请求
                ret = glex_rdma(ep,&right_rdmaReq,NULL);
                TEST_RetSuccess(ret,"非阻塞RDMA写失败！");

                // 轮询检查RDMA操作是否出现错误请求
                uint32_t num_er = 0;
                struct glex_err_req *er_list;
                ret = glex_poll_error_req(ep,&num_er,er_list);
                if(num_er != 0){
                    printf("rank : %d, %s, 出错的个数为%d\n",rank,"glex_rdma_put_counter操作失败，num_er不为0.",num_er);
                    printf("出错的原因为：%s\n",glex_error_req_status_str(er_list[0].status));
                }
            }
            //glex_rdma_recv(rank,2,trans_rank[i][j - 1],trans_rank[i - 1][j],ep);

            //printf("接收节点：%d, 接收后，buffer内容是，%s\n",rank, mem_addr);
        }
        
        MPI_Barrier(MPI_COMM_WORLD);

  }

    //当前节点既有左节点又有上节点
    //printf("k : %d\n",k);    
    if (i - 1 >= 0 && j - 1 >= 0){
        //当前节点既没有右节点也没有下节点，这是终点
        if (i + 1 >= m && j + 1 >= n){
            t_end = MPI_Wtime();

            printf("执行时间为 %lf\n", (t_end - t_start) * 1e6 / iterations);
        }
    }

    //printf("rank : %d\n",rank);

   

    /* 其他工作 */

   MPI_Finalize();
   //free(event);

   /* 释放内存 */
   ret = glex_deregister_mem(ep,mh);
   free(mem_addr);

   /* 释放端点资源 */
   ret = glex_destroy_ep(ep);

   /* 关闭GLEX设备 */
   ret = glex_close_device(dev);
   
   return 0;
}





