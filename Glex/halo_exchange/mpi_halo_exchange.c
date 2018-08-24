#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "math.h"
#include "mpi.h"

int main(int argc,char** argv)
{
    int          taskid, ntasks;
    MPI_Status   status[4];
    MPI_Request  send_request[4],recv_request[4];
    int          ierr,i,j;
    int          buffsize;
    double       *sendbuff,*recvbuff1,*recvbuff2,*recvbuff3,*recvbuff4;
    double       inittime,recvtime,totaltime;
   
    /* MPI初始化 */
    MPI_Init(&argc, &argv);

    /* 获得MPI进程数量以及进程编号 */
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Comm_size(MPI_COMM_WORLD,&ntasks);

    /* 从程序运行参数中获得二维进程矩阵边长和buffer的大小 */
    int row=atoi(argv[1]);			//行
    int col=atoi(argv[2]);			//列
    buffsize=atoi(argv[3]);

    /* 申请内存 */ 
    sendbuff=(double *)malloc(sizeof(double)*buffsize);
    recvbuff1=(double *)malloc(sizeof(double)*buffsize);
    recvbuff2=(double *)malloc(sizeof(double)*buffsize);
    recvbuff3=(double *)malloc(sizeof(double)*buffsize);
    recvbuff4=(double *)malloc(sizeof(double)*buffsize);
    
    /* 向量初始化 */
    srand((unsigned)time( NULL ) + taskid);
    for(i=0;i<buffsize;i++){
        sendbuff[i]=(double)rand()/RAND_MAX;
    }

    /* 进行通信,发送时需要注意上下左右边界问题 */
    int loop = 0;
    totaltime = 0;
    MPI_Barrier(MPI_COMM_WORLD); 
    inittime = MPI_Wtime();
    for(loop=0;loop<1000;loop++){
        // 向左进程发送与接收
        if(taskid != 0 && (taskid-1)/col == taskid/col){
        	//printf("rank %d 的左进程为 %d\n", taskid, taskid-1);
        	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid-1,0,MPI_COMM_WORLD,&send_request[0]);
        	ierr = MPI_Irecv(recvbuff1,buffsize,MPI_DOUBLE,taskid-1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[0]);
        }else{
        	//printf("rank %d 的左进程为 %d\n", taskid, taskid+col-1);
        	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+col-1,0,MPI_COMM_WORLD,&send_request[0]);
        	ierr = MPI_Irecv(recvbuff1,buffsize,MPI_DOUBLE,taskid+col-1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[0]);
        }
        // 向右进程发送与接收
        if((taskid+1)/col == taskid/col){
        	//printf("rank %d 的右进程为 %d\n", taskid, taskid+1);
        	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+1,0,MPI_COMM_WORLD,&send_request[1]);
        	ierr = MPI_Irecv(recvbuff2,buffsize,MPI_DOUBLE,taskid+1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[1]);
        }else{
        	//printf("rank %d 的右进程为 %d\n", taskid, taskid-col+1);
        	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid-col+1,0,MPI_COMM_WORLD,&send_request[1]);
        	ierr = MPI_Irecv(recvbuff2,buffsize,MPI_DOUBLE,taskid-col+1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[1]);
        }
        // 向上进程发送与接收
        if((taskid-col)>=0){
        	//printf("rank %d 的上进程为 %d\n", taskid, taskid-col);
        	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid-col,0,MPI_COMM_WORLD,&send_request[2]);
        	ierr = MPI_Irecv(recvbuff3,buffsize,MPI_DOUBLE,taskid-col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[2]);
        }else{
        	//printf("rank %d 的上进程为 %d\n", taskid, taskid+(row-1)*col);
        	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+(row-1)*col,0,MPI_COMM_WORLD,&send_request[2]);
        	ierr = MPI_Irecv(recvbuff3,buffsize,MPI_DOUBLE,taskid+(row-1)*col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[2]);
        }
        // 向下进程发送与接收
        if((taskid+col)<ntasks){
        	//printf("rank %d 的下进程为 %d\n", taskid, taskid+col);
        	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+col,0,MPI_COMM_WORLD,&send_request[3]);
        	ierr = MPI_Irecv(recvbuff4,buffsize,MPI_DOUBLE,taskid+col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[3]);
        }else{
        	//printf("rank %d 的下进程为 %d\n", taskid, taskid-(row-1)*col);
        	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid-(row-1)*col,0,MPI_COMM_WORLD,&send_request[3]);
        	ierr = MPI_Irecv(recvbuff4,buffsize,MPI_DOUBLE,taskid-(row-1)*col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request[3]);
        }

        /* 等待通信结束，计时 */
        ierr = MPI_Waitall(4, send_request, status);
        ierr = MPI_Waitall(4, recv_request, status);

        MPI_Barrier(MPI_COMM_WORLD);
    }
    if(taskid == 0){
        recvtime = MPI_Wtime();
        double totaltime = (recvtime - inittime) * 1e6 / (2.0 * 1000);
        printf("MPI_HALO_EXCHANGE: Trans %d Bytes,",buffsize*8);
        printf("totaltime %.5lf us\n", totaltime);
    }

    /* 释放资源 */
    free(recvbuff1);
    free(recvbuff2);
    free(recvbuff3);
    free(recvbuff4);
    free(sendbuff);

    /* MPI程序结束 */
    MPI_Finalize();
}