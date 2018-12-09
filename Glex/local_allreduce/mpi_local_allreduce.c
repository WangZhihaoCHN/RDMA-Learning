#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "math.h"
#include "mpi.h"

int main(int argc,char** argv)
{
    int          taskid, ntasks;
    MPI_Status   status;
    MPI_Request  handle;
    int          ierr,i,j;
    int          buffsize;
    char         *sendbuff,*recvbuff;
    double       inittime,recvtime,totaltime;
   
    /* MPI初始化 */
    MPI_Init(&argc, &argv);

    /* 获得MPI进程数量以及进程编号 */
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Comm_size(MPI_COMM_WORLD,&ntasks);

    /* 从程序运行参数中获得二维进程矩阵边长和buffer的大小 */
    buffsize=atoi(argv[1]);

    /* 申请内存 */ 
    sendbuff=(char *)malloc(sizeof(char)*buffsize);
    recvbuff=(char *)malloc(sizeof(char)*buffsize*ntasks);
    
    /* 向量初始化 */
    srand((unsigned)time(NULL)+taskid);
    for(i=0;i<buffsize;i++){
        sendbuff[i] = rand()%256;
    }

    /* 进行通信,发送时需要注意上下左右边界问题 */
    int loop = 0;
    totaltime = 0;
    MPI_Barrier(MPI_COMM_WORLD); 
    inittime = MPI_Wtime();

    for(loop=0;loop<1000;loop++){
        MPI_Allgather(sendbuff, buffsize, MPI_CHAR, recvbuff, buffsize, MPI_CHAR, MPI_COMM_WORLD);
        /* 等待通信结束，计时 */
        MPI_Barrier(MPI_COMM_WORLD);
    }
    if(taskid == 0){
        recvtime = MPI_Wtime();
        double totaltime = (recvtime - inittime) * 1e6 / 1000;
        printf("MPI_LOCAL_ALLREDUCE: Trans %d Bytes,",buffsize);
        printf("totaltime %.5lf us\n", totaltime);
    }

    /* 释放资源 */
    free(recvbuff);
    free(sendbuff);

    /* MPI程序结束 */
    MPI_Finalize();
}