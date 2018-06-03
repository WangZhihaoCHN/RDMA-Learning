#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "math.h"
#include "mpi.h"

int main(int argc,char** argv)
{
    int          taskid, ntasks;
    MPI_Status   status;
    MPI_Request  send_request1,send_request2,send_request3,send_request4;
    MPI_Request  recv_request1,recv_request2,recv_request3,recv_request4;
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
    MPI_Barrier(MPI_COMM_WORLD); 
    inittime = MPI_Wtime();
    // 向左进程发送与接收
    if(taskid != 0 && (taskid-1)/col == taskid/col){
    	//printf("rank %d 的左进程为 %d\n", taskid, taskid-1);
    	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid-1,0,MPI_COMM_WORLD,&send_request1);
    	ierr = MPI_Irecv(recvbuff1,buffsize,MPI_DOUBLE,taskid-1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request1);
    }else{
    	//printf("rank %d 的左进程为 %d\n", taskid, taskid+col-1);
    	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+col-1,0,MPI_COMM_WORLD,&send_request1);
    	ierr = MPI_Irecv(recvbuff1,buffsize,MPI_DOUBLE,taskid+col-1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request1);
    }
    // 向右进程发送与接收
    if((taskid+1)/col == taskid/col){
    	//printf("rank %d 的右进程为 %d\n", taskid, taskid+1);
    	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+1,0,MPI_COMM_WORLD,&send_request2);
    	ierr = MPI_Irecv(recvbuff2,buffsize,MPI_DOUBLE,taskid+1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request2);
    }else{
    	//printf("rank %d 的右进程为 %d\n", taskid, taskid-col+1);
    	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid-col+1,0,MPI_COMM_WORLD,&send_request2);
    	ierr = MPI_Irecv(recvbuff2,buffsize,MPI_DOUBLE,taskid-col+1,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request2);
    }
    // 向上进程发送与接收
    if((taskid-col)>=0){
    	//printf("rank %d 的上进程为 %d\n", taskid, taskid-col);
    	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid-col,0,MPI_COMM_WORLD,&send_request3);
    	ierr = MPI_Irecv(recvbuff3,buffsize,MPI_DOUBLE,taskid-col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request3);
    }else{
    	//printf("rank %d 的上进程为 %d\n", taskid, taskid+(row-1)*col);
    	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+(row-1)*col,0,MPI_COMM_WORLD,&send_request3);
    	ierr = MPI_Irecv(recvbuff3,buffsize,MPI_DOUBLE,taskid+(row-1)*col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request3);
    }
    // 向下进程发送与接收
    if((taskid+col)<ntasks){
    	//printf("rank %d 的下进程为 %d\n", taskid, taskid+col);
    	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid+col,0,MPI_COMM_WORLD,&send_request4);
    	ierr = MPI_Irecv(recvbuff4,buffsize,MPI_DOUBLE,taskid+col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request4);
    }else{
    	//printf("rank %d 的下进程为 %d\n", taskid, taskid-(row-1)*col);
    	ierr = MPI_Isend(sendbuff,buffsize,MPI_DOUBLE,taskid-(row-1)*col,0,MPI_COMM_WORLD,&send_request4);
    	ierr = MPI_Irecv(recvbuff4,buffsize,MPI_DOUBLE,taskid-(row-1)*col,MPI_ANY_TAG,MPI_COMM_WORLD,&recv_request4);
    }

    /* 等待通信结束，计时 */
    ierr=MPI_Wait(&send_request1,&status);
    ierr=MPI_Wait(&recv_request1,&status);
    ierr=MPI_Wait(&send_request2,&status);
    ierr=MPI_Wait(&recv_request2,&status);
    ierr=MPI_Wait(&send_request3,&status);
    ierr=MPI_Wait(&recv_request3,&status);
    ierr=MPI_Wait(&send_request4,&status);
    ierr=MPI_Wait(&recv_request4,&status);

    MPI_Barrier(MPI_COMM_WORLD); 
	recvtime = MPI_Wtime();
    totaltime = recvtime - inittime;
	printf("二维影响区交换已经完成，用时 %.4lf ms\n", totaltime*1000);

    /* 释放资源 */
    free(recvbuff1);
    free(recvbuff2);
    free(recvbuff3);
    free(recvbuff4);
    free(sendbuff);

    /* MPI程序结束 */
    MPI_Finalize();

}