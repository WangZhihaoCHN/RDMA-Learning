#include <stdio.h>
#include <mpi.h>
#include <string.h>

//glex中的rdma最大传输长度128M


int main(int argc,char* argv[]){

    //传进来的消息大小、行、列
    int message_size,m,n;
    sscanf(argv[argc - 3],"%d",&m);
    sscanf(argv[argc - 2],"%d",&n);
    sscanf(argv[argc - 1],"%d",&message_size);

    /* 其中保存着对应矩阵位置的进程号 */
    int trans[100][100];
    int left_mess_size[100][100];
    int up_mess_size[100][100];
    int total_mess_size[100][100];
    int i = 0,j = 0,num = 0;
    for (i = 0;i < m; i++){
        for (j = 0;j < n;j++){
            trans[i][j] = num;
            num++;

            //当前节点既没有左节点也没有上节点，即开始节点
            if (i - 1 < 0 && j - 1 < 0){
                left_mess_size[i][j] = 0;
                up_mess_size[i][j] = 0;
                total_mess_size[i][j] = left_mess_size[i][j] + up_mess_size[i][j] + message_size;
                continue;
            }
            
            //当前节点只有上节点
            if (i - 1 >= 0 && j - 1 < 0){
                up_mess_size[i][j] = total_mess_size[i - 1][j];
                left_mess_size[i][j] = 0;
                total_mess_size[i][j] = up_mess_size[i][j] + left_mess_size[i][j] + message_size;
                continue;
            }

            //当前节点只有左节点
            if (j - 1 >= 0 && i - 1 < 0){
                left_mess_size[i][j] = total_mess_size[i][j - 1];
                up_mess_size[i][j] = 0;
                total_mess_size[i][j] = left_mess_size[i][j] + up_mess_size[i][j] + message_size;
                continue;
            }

            //当前节点既有左节点又有上节点
            if (i - 1 >= 0 && j - 1 >= 0){
                left_mess_size[i][j] = total_mess_size[i][j - 1];
                up_mess_size[i][j] = total_mess_size[i - 1][j];
                total_mess_size[i][j] = up_mess_size[i][j] + left_mess_size[i][j] + message_size;
            }

        }
    }
   
        
    //printf("%d %d %d\n",message_size,m,n);
    //return 0;
    //进程号和进程数
    int rank,size;

    MPI_Status status;
    
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
   
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
    char* send_msg = (char *)malloc((total_mess_size[i][j] + 1) * sizeof(char));
    char* recv_msg_left = (char *)malloc((left_mess_size[i][j] + 1) * sizeof(char));
    char* recv_msg_up = (char *)malloc((up_mess_size[i][j] + 1)* sizeof(char));
    
    
    char c[1];
    sprintf(c,"%d",rank);
    int k = 0;
    memset(send_msg,0,total_mess_size[i][j] + 1);
    for (;k < message_size;k++) send_msg[k] = c[0];
    //send_msg[message_size] = 0;
    memset(recv_msg_left,0,left_mess_size[i][j] + 1);
    memset(recv_msg_up,0,up_mess_size[i][j] + 1);
    //printf("rank : %d, %s, %d\n",rank,send_msg,total_mess_size[i][j]);
    //printf("rank : %d, total_mess : %s, left_mess : %s, up_mess : %s\n",rank,send_msg,recv_msg_left,recv_msg_up);
    //MPI_Finalize();
    //return 0;


    int iterations = 0;
    int skip = 0;

    //printf("rank : %d , left_mess_size=%d,up_mess_size=%d\n",rank,left_mess_size[i][j],up_mess_size[i][j]);
    //MPI_Finalize();
    //return 0;

    if (message_size > 8192){
        iterations = 30;
        skip = 2;
    }else{
        iterations = 100;
        skip = 10;
    }
    
    double t_start = 0.0,t_end = 0.0;
    MPI_Barrier(MPI_COMM_WORLD);
    //t_start = MPI_Wtime();
    for (k = 0; k < iterations + skip; k++){
        if (k == skip && rank == trans[m - 1][n - 1]){
            t_start = MPI_Wtime();
        }
        
        //判断当前节点是否有左节点与其相连
        if (j - 1 >= 0){
            //当前节点有左节点与其相连 需要接收到左节点的信息
            MPI_Recv(recv_msg_left,left_mess_size[i][j],MPI_CHAR,trans[i][j - 1],0,MPI_COMM_WORLD,&status);
            strcat(send_msg,recv_msg_left);
            //printf("进程 : %d, 接收的左节点的消息为: %s\n",rank,recv_msg_left);
        }
        
        //判断当前节点是否有上节点与其相连
        if (i - 1 >= 0){
            //当前节点有上节点与其相连，需要接收到上节点的信息
            MPI_Recv(recv_msg_up,up_mess_size[i][j],MPI_CHAR,trans[i - 1][j],0,MPI_COMM_WORLD,&status);
            strcat(send_msg,recv_msg_up);
            //printf("进程 : %d, 接收的上节点的消息为 : %s\n",rank,recv_msg_up);
        }
        
        //判断当前节点是否有右节点与其相连
        if (j + 1 < n){
            //当前节点有右节点与其相连，需要将信息发送给右节点
            MPI_Send(send_msg,total_mess_size[i][j],MPI_CHAR,trans[i][j + 1],0,MPI_COMM_WORLD);
            //printf("rank : %d, 发送给右节点的消费为 : %s\n",rank,send_msg);
        }
        
        //判断当前节点是否有下节点与其相连
        if (i + 1 < m){
            //当前节点有下节点与其相连，需要将信息发送给下节点
            MPI_Send(send_msg,total_mess_size[i][j],MPI_CHAR,trans[i + 1][j],0,MPI_COMM_WORLD);
            //printf("rank : %d, 发送给下节点的消息为 : %s\n",rank,send_msg);
        }
        
        //清除数组中的数据
        memset(send_msg,0,total_mess_size[i][j] + 1);
        memset(send_msg,c[0],message_size);
        memset(recv_msg_left,0,left_mess_size[i][j] + 1);
        memset(recv_msg_up,0,up_mess_size[i][j] + 1);
        MPI_Barrier(MPI_COMM_WORLD);
    
    }

    if (rank == trans[m - 1][n - 1]){
        t_end = MPI_Wtime();

        double latency = (t_end - t_start) * 1e6 / iterations / 1000;
        //printf("进程 : %d, 接收的上节点的消息为 : %s\n",rank,recv_msg_up);
        //printf("进程 : %d, 接收的左节点的消息为: %s\n",rank,recv_msg_left);
        printf("进程 : %d，执行的时间为 : %lf ms\n",rank,latency);
   }

    MPI_Finalize();

    return 0;
}
