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

void  TEST_RetSuccess(glex_ret_t glex_ret,char* str){
      if(strcmp(glex_error_str(glex_ret),"successful") != 0){
          printf("Err :%s--%s\n",glex_error_str(glex_ret),str);
          exit(EXIT_FAILURE);
          return;
      }
 }

int main(int argc,char **argv){
 
    

    //获得传输的消息大小、矩阵的长和宽
    int message_size,m,n;
    sscanf(argv[argc - 1],"%d",&message_size);
    sscanf(argv[argc - 2],"%d",&n);
    sscanf(argv[argc - 3],"%d",&m);

    /* 其中保存着对应矩阵位置的进程号 */
    int trans_rank[100][100];
    /* 当前进程需要get的进程数 */
    int get_num[100][100]; 
    int i = 0,j = 0,num = 0;
    for (i = 0;i < m; i++){
        for (j = 0;j < n;j++){
            trans_rank[i][j] = num;
            num++;

            //当前节点既没有左节点也没有上节点，即开始节点
            if (i - 1 < 0 && j - 1 < 0){
                get_num[i][j] = 0;
                continue;
            }
            
            //当前节点只有上节点
            if (i - 1 >= 0 && j - 1 < 0){
                get_num[i][j] = get_num[i - 1][j] + 1;
                continue;
            }

            //当前节点只有左节点
            if (j - 1 >= 0 && i - 1 < 0){
                get_num[i][j] = get_num[i][j - 1] + 1;
                continue;
            }

            //当前节点既有左节点又有上节点
            if (i - 1 >= 0 && j - 1 >= 0){
                get_num[i][j] = get_num[i - 1][j] + get_num[i][j - 1] + 2;
            }

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
    
    
    //进程号，开启的进程数
    int size;
    MPI_Status status;

    //初始化MPI环境
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);


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
 	};
    

 	ret = glex_create_ep(dev,&ep_attr,&ep);

 	/* 获得指定端点的端点地址 */
 	ret = glex_get_ep_addr(ep,&ep_addr);


 	/* 注册内存 */

    char* send_msg = (char *)malloc((message_size + 1) * sizeof(char));
    
    char c[1];
    sprintf(c,"%d",rank);
    
    //strcpy(send_msg,c);
    memset(send_msg,0,message_size + 1);
    memset(send_msg,c[0],message_size);
    
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

	//printf("rank : %d, 需要get的次数为%d\n",rank,get_num[i][j]);    
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


 	/* 利用GLEX，在本地端点和远程端点之间。进行RDMA操作 */

    
    double t_start = MPI_Wtime(),t_end = 0.0;

    int iterations = 0,skip = 0;

    if (message_size > 8192){
        iterations = 1000;
        skip = 10;
    }else{
        iterations = 10000;
        skip = 100;
    }

    int k = 0,d = 0;

    
    MPI_Barrier(MPI_COMM_WORLD);
    for (k = 0;k < 10;k++){
      
      //if (k == skip && rank == trans_rank[m - 1][n - 1]){
      //     t_start = MPI_Wtime();
      
      //}
      
        //当前节点既没有左节点也没有上节点，设置自己的counter为0
        if (j - 1 < 0 && i - 1 < 0){
            
            //当前节点既有右节点也有下节点
            struct glex_imm_mp_req down_mpReq = {
                .rmt_ep_addr = down_remote_ep_addr,
                .data = send_msg,
                .len = message_size,
                //.coll_counter = 0,
                .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA | GLEX_FLAG_FENCE,
                .next = NULL
            };

            struct glex_imm_mp_req right_mpReq = {
                .rmt_ep_addr = right_remote_ep_addr,
                .data = send_msg,
                .len = message_size,
                .coll_counter = 0,
                .flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA | GLEX_FLAG_FENCE,
                .next = &down_mpReq
            };

            ret = glex_send_imm_mp(ep,&right_mpReq,NULL);
            TEST_RetSuccess(ret,"非阻塞发送MP报文失败");
            
                
        }
        
        //当前节点只有左节点,设置自己的counter为1
        if (j - 1 >= 0 && i - 1 < 0){
            //当前节点只有下节点
            if (i + 1 < m && j + 1 >= n){

            	struct glex_imm_mp_req mpReq[100];
            	int t = 0;
            	for (t = 0;t < get_num[i][j];t++){
            		mpReq[t].rmt_ep_addr = down_remote_ep_addr;
                    mpReq[t].data = send_msg;
                    mpReq[t].len = message_size;
            		mpReq[t].coll_counter = 1;
            		mpReq[t].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		if (t == get_num[i][j] - 1) mpReq[t].next = NULL;
            		else mpReq[t].next = (mpReq + t + 1);
            	}

            	mpReq[t].rmt_ep_addr = down_remote_ep_addr;
            	mpReq[t].data = send_msg;
            	mpReq[t].len = message_size;
            	mpReq[t].coll_counter = 0;
            	mpReq[t].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            	mpReq[t].next = NULL;

            	mpReq[t - 1].next = mpReq + t;

                ret = glex_send_imm_mp(ep,mpReq,NULL);
                TEST_RetSuccess(ret,"非阻塞发送MP报文失败");

            }
            //当前节点既有右节点也有下节点
            if (i + 1 < m && j + 1 < n){

            	struct glex_imm_mp_req mpReq[100];
            	int t = 0;
            	int u = 0;
            	for (t = 0;t < get_num[i][j];t++){
            		mpReq[u + 1].rmt_ep_addr = down_remote_ep_addr;
                    mpReq[u + 1].data = send_msg;
                    mpReq[u + 1].len = message_size;
            		//mpReq[u].coll_counter = 1;
            		mpReq[u + 1].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		if (t == get_num[i][j] - 1) mpReq[u + 1].next = NULL;
            		else mpReq[u + 1].next = (mpReq + u + 2);

            		mpReq[u].rmt_ep_addr = right_remote_ep_addr;
                    mpReq[u].data = send_msg;
                    mpReq[u].len = message_size;
            		mpReq[u].coll_counter = 1;
            		mpReq[u].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		mpReq[u].next = mpReq + u + 1;

            		u += 2;

            	}
            
                
                mpReq[u + 1].rmt_ep_addr = down_remote_ep_addr;
                mpReq[u + 1].data = send_msg;
                mpReq[u + 1].len = message_size;
                mpReq[u + 1].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
                mpReq[u + 1].next = NULL;

                mpReq[u].rmt_ep_addr = right_remote_ep_addr;
                mpReq[u].data = send_msg;
                mpReq[u].len = message_size;
                mpReq[u].coll_counter = 0;
                mpReq[u].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
                mpReq[u].next = mpReq + u + 1;

                mpReq[u - 1].next = mpReq + u;

                ret = glex_send_imm_mp(ep,mpReq,NULL);
                TEST_RetSuccess(ret,"非阻塞发送MP报文失败");

            }

            
        }
        //当前节点只有上节点，设置自己的counter为1
        if (i - 1 >= 0 && j - 1 < 0){
            //当前节点只有右节点
            if (j + 1 < n && i + 1 >= m){

                struct glex_imm_mp_req mpReq[100];
            	int t = 0;
            	for (t = 0;t < get_num[i][j];t++){
            		mpReq[t].rmt_ep_addr = right_remote_ep_addr;
                    mpReq[t].data = send_msg;
                    mpReq[t].len = message_size;
            		mpReq[t].coll_counter = 1;
            		mpReq[t].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		if (t == get_num[i][j] - 1) mpReq[t].next = NULL;
            		else mpReq[t].next = (mpReq + t + 1);
            	}

            	mpReq[t].rmt_ep_addr = right_remote_ep_addr;
            	mpReq[t].data = send_msg;
            	mpReq[t].len = message_size;
            	mpReq[t].coll_counter = 0;
            	mpReq[t].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            	mpReq[t].next = NULL;

            	mpReq[t - 1].next = mpReq + t;

                ret = glex_send_imm_mp(ep,mpReq,NULL);
                TEST_RetSuccess(ret,"非阻塞发送MP报文失败");

            }
            //当前节点既有右节点也有下节点
            if (i + 1 < m && j + 1 < n){
                struct glex_imm_mp_req mpReq[100];
            	int t = 0;
            	int u = 0;
            	for (t = 0;t < get_num[i][j];t++){
            		mpReq[u + 1].rmt_ep_addr = down_remote_ep_addr;
                    mpReq[u + 1].data = send_msg;
                    mpReq[u + 1].len = message_size;
            		//mpReq[u].coll_counter = 1;
            		mpReq[u + 1].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		if (t == get_num[i][j] - 1) mpReq[u + 1].next = NULL;
            		else mpReq[u + 1].next = (mpReq + u + 2);

            		mpReq[u].rmt_ep_addr = right_remote_ep_addr;
                    mpReq[u].data = send_msg;
                    mpReq[u].len = message_size;
            		mpReq[u].coll_counter = 1;
            		mpReq[u].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		mpReq[u].next = mpReq + u + 1;

            		u += 2;

            	}
            
                
                mpReq[u + 1].rmt_ep_addr = down_remote_ep_addr;
                mpReq[u + 1].data = send_msg;
                mpReq[u + 1].len = message_size;
                mpReq[u + 1].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
                mpReq[u + 1].next = NULL;

                mpReq[u].rmt_ep_addr = right_remote_ep_addr;
                mpReq[u].data = send_msg;
                mpReq[u].len = message_size;
                mpReq[u].coll_counter = 0;
                mpReq[u].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
                mpReq[u].next = mpReq + u + 1;

                mpReq[u - 1].next = mpReq + u;

                ret = glex_send_imm_mp(ep,mpReq,NULL);
                TEST_RetSuccess(ret,"非阻塞发送MP报文失败");

                
            }
            
        }
        //当前节点既有左节点又有上节点，设置自己的counter为2
        if (i - 1 >= 0 && j - 1 >= 0){
            //当前节点既没有右节点也没有下节点，这是终点
           
            if (i + 1 >= m && j + 1 >= n){
                //当接收到相应的事件后显示内存中的内容
                
                glex_ep_addr_t source_ep_addr;
                void** tmp_mem = (void **)malloc(50*sizeof(void*));
                memset(tmp_mem,0,50);
                int tmp_len = 0;
                while(1) {

                    ret = glex_probe_first_mp(ep,5,&source_ep_addr,tmp_mem,&tmp_len);
                    if (strcmp(glex_error_str(ret),"successful") != 0){
                        break;
                    }
                    //printf("rank : %d, mp报文的数据为:%s\n",rank,(char *)*tmp_mem);
                    glex_discard_probed_mp(ep);
                    tmp_len = 0;
                    memset(tmp_mem,0,50);
                }
                
                //printf("接收节点：%d, 接收后，buffer内容是，%s\n",rank, mem_addr);
                //t_end = MPI_Wtime();
                //glex_rdma_recv(rank,2,trans_rank[i][j - 1],trans_rank[i - 1][j],ep);
                //printf("执行时间为 %lf\n", (t_end - t_start) * 1e6);

            }
            
            //当前节点只有右节点
            if (j + 1 < n && i + 1 >= m){

                struct glex_imm_mp_req mpReq[100];
            	int t = 0;
            	for (t = 0;t < get_num[i][j];t++){
            		mpReq[t].rmt_ep_addr = right_remote_ep_addr;
                    mpReq[t].data = send_msg;
                    mpReq[t].len = message_size;
            		mpReq[t].coll_counter = 1;
            		mpReq[t].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		if (t == get_num[i][j] - 1) mpReq[t].next = NULL;
            		else mpReq[t].next = (mpReq + t + 1);
            	}

            	mpReq[t].rmt_ep_addr = right_remote_ep_addr;
            	mpReq[t].data = send_msg;
            	mpReq[t].len = message_size;
            	mpReq[t].coll_counter = 0;
            	mpReq[t].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            	mpReq[t].next = NULL;

            	mpReq[t - 1].next = mpReq + t;

                ret = glex_send_imm_mp(ep,mpReq,NULL);
                TEST_RetSuccess(ret,"非阻塞发送MP报文失败");
                
            }
            //当前节点只有下节点
            if (i + 1 < m && j + 1 >= n){
                struct glex_imm_mp_req mpReq[100];
            	int t = 0;
            	for (t = 0;t < get_num[i][j];t++){
            		mpReq[t].rmt_ep_addr = down_remote_ep_addr;
                    mpReq[t].data = send_msg;
                    mpReq[t].len = message_size;
            		mpReq[t].coll_counter = 1;
            		mpReq[t].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		if (t == get_num[i][j] - 1) mpReq[t].next = NULL;
            		else mpReq[t].next = (mpReq + t + 1);
            	}

            	mpReq[t].rmt_ep_addr = down_remote_ep_addr;
            	mpReq[t].data = send_msg;
            	mpReq[t].len = message_size;
            	mpReq[t].coll_counter = 0;
            	mpReq[t].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            	mpReq[t].next = NULL;

            	mpReq[t - 1].next = mpReq + t;

                ret = glex_send_imm_mp(ep,mpReq,NULL);
                TEST_RetSuccess(ret,"非阻塞发送MP报文失败");
                
            }
            //当前节点既有右节点也有下节点
            if (i + 1 < m && j + 1 < n){
                struct glex_imm_mp_req mpReq[100];
            	int t = 0;
            	int u = 0;
            	for (t = 0;t < get_num[i][j];t++){
            		mpReq[u + 1].rmt_ep_addr = down_remote_ep_addr;
                    mpReq[u + 1].data = send_msg;
                    mpReq[u + 1].len = message_size;
            		//mpReq[u].coll_counter = 1;
            		mpReq[u + 1].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		if (t == get_num[i][j] - 1) mpReq[u + 1].next = NULL;
            		else mpReq[u + 1].next = (mpReq + u + 2);

            		mpReq[u].rmt_ep_addr = right_remote_ep_addr;
                    mpReq[u].data = send_msg;
                    mpReq[u].len = message_size;
            		mpReq[u].coll_counter = 1;
            		mpReq[u].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_SWAP | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
            		mpReq[u].next = mpReq + u + 1;

            		u += 2;

            	}
            
                
                mpReq[u + 1].rmt_ep_addr = down_remote_ep_addr;
                mpReq[u + 1].data = send_msg;
                mpReq[u + 1].len = message_size;
                mpReq[u + 1].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_TAIL | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
                mpReq[u + 1].next = NULL;

                mpReq[u].rmt_ep_addr = right_remote_ep_addr;
                mpReq[u].data = send_msg;
                mpReq[u].len = message_size;
                mpReq[u].coll_counter = 0;
                mpReq[u].flag = GLEX_FLAG_COLL | GLEX_FLAG_COLL_SEQ_START | GLEX_FLAG_COLL_COUNTER | GLEX_FLAG_COLL_CP_SWAP_DATA | GLEX_FLAG_COLL_CP_DATA| GLEX_FLAG_FENCE;
                mpReq[u].next = mpReq + u + 1;

                mpReq[u - 1].next = mpReq + u;

                ret = glex_send_imm_mp(ep,mpReq,NULL);
                TEST_RetSuccess(ret,"非阻塞发送MP报文失败");
                
            }
            
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
   //ret = glex_deregister_mem(ep,mh);
   free(send_msg);

   /* 释放端点资源 */
   ret = glex_destroy_ep(ep);

   /* 关闭GLEX设备 */
   ret = glex_close_device(dev);
   
   return 0;
}





