#include <stdio.h>
#include <mpi.h>
#include <string.h>

//glex中的rdma最大传输长度128M
const int maxn = 134217728;

int main(int argc,char* argv[]){

    //传进来的消息大小、行、列
    int message_size,m,n;
    sscanf(argv[argc - 3],"%d",&m);
    sscanf(argv[argc - 2],"%d",&n);
    sscanf(argv[argc - 1],"%d",&message_size);
   
    //printf("%d %d %d\n",message_size,m,n);
    //return 0;
    //进程号和进程数
    int rank,size;

    MPI_Status status;
    
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    char send_msg[maxn];
    char recv_msg_left[maxn];
    char recv_msg_up[maxn];

    char c[1];
    sprintf(c,"%d",rank);
    memset(send_msg,c[0],message_size);   //将当前进程号当做信息发送
    
        
    /* 其中保存着对应矩阵位置的进程号 */
    int trans[100][100];
    int i = 0,j = 0,num = 0;
    for (i = 0;i < m; i++){
        for (j = 0;j < n;j++){
            trans[i][j] = num;
            num++;
        }
    }
    
    int b = 0;
    //寻找当前节点所在的位置，即处于矩阵的行数和列数
    for (i = 0; i < m;i++){
    
        for (j = 0; j < n;j++){
            if (rank == trans[i][j]){
                b = 1;
                break;
            }
        }
        if (b) break;
    }
    //当前进程所处在的矩阵的行数为i，列数为j

    int iterations = 0;
    int skip = 0;

    //printf("rank : %d , i=%d,j=%d\n",rank,i,j);
    //MPI_Finalize();
    //return 0;

    if (message_size > 8192){
        iterations = 1000;
        skip = 10;
    }else{
        iterations = 10000;
        skip = 100;
    }
    
    double t_start = 0.0,t_end = 0.0;
    int k = 0;
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    for (; k < iterations + skip; k++){
        /*
        if (k == skip && rank == trans[m - 1][n - 1]){
            t_start = MPI_Wtime();
        }
        */
        //判断当前节点是否有左节点与其相连
        if (j - 1 >= 0){
            //当前节点有左节点与其相连 需要接收到左节点的信息
            MPI_Recv(recv_msg_left,message_size,MPI_CHAR,trans[i][j - 1],0,MPI_COMM_WORLD,&status);
            //printf("进程 : %d, 接收的左节点的消息为: %s\n",rank,recv_msg_left);
        }
        
        //判断当前节点是否有上节点与其相连
        if (i - 1 >= 0){
            //当前节点有上节点与其相连，需要接收到上节点的信息
            MPI_Recv(recv_msg_up,message_size,MPI_CHAR,trans[i - 1][j],0,MPI_COMM_WORLD,&status);
            //printf("进程 : %d, 接收的上节点的消息为 : %s\n",rank,recv_msg_up);
        }
        
        //判断当前节点是否有右节点与其相连
        if (j + 1 < n){
            //当前节点有右节点与其相连，需要将信息发送给右节点
            MPI_Send(send_msg,message_size,MPI_CHAR,trans[i][j + 1],0,MPI_COMM_WORLD);
        }
        
        //判断当前节点是否有下节点与其相连
        if (i + 1 < m){
            //当前节点有下节点与其相连，需要将信息发送给下节点
            MPI_Send(send_msg,message_size,MPI_CHAR,trans[i + 1][j],0,MPI_COMM_WORLD);
        }
        
        MPI_Barrier(MPI_COMM_WORLD);
    
    }

    if (rank == trans[m - 1][n - 1]){
        t_end = MPI_Wtime();

        double latency = (t_end - t_start) * 1e6 / iterations;
        //printf("进程 : %d, 接收的上节点的消息为 : %s\n",rank,recv_msg_up);
        //printf("进程 : %d, 接收的左节点的消息为: %s\n",rank,recv_msg_left);
        printf("进程 : %d，执行的时间为 : %lf\n",rank,latency);
   }

    MPI_Finalize();

    return 0;
}
