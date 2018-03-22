#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"

#define SIZE 40000

int main(int argc, char *argv[])
{
	double SEC = 0;
	if(argc == 2){
		SEC = atof(argv[1]);
	}else if(argc > 2){
		printf("输入参数错误！");
	}

	int num_procs,my_id,flag;	//进程总数、进程ID、传输是否完成标识
	int buf[SIZE][SIZE];		//发送缓冲区
	MPI_Status status;			//status对象(Status)
	MPI_Request handle;			//MPI请求(request)

	//记录MPI_Isend()开始时间，cpu占用结束时间，MPI_Wait()结束时间
	double startTime,cpuTime,waitTime,totalTime;		

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&num_procs);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_id);

	//两个进程，进程1非阻塞发送，进程0阻塞接收
	if(my_id == 1){

		MPI_Isend(buf, SIZE*SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, &handle);
		
		startTime = MPI_Wtime();
		//printf("Task start sending the array : %lf seconds \n", startTime);
		
		printf("发送进程: 占用CPU %.4lf 秒\n", SEC);

		//占用CPU一段时间（1s）
		clock_t st, rt;				//占用cpu开始时间，占用cpu当前执行时间
		st = clock();
		double cps = (double)(SEC * CLOCKS_PER_SEC);  //需要占用SEC秒数
		while(1){
			rt = clock();
			if( (double)(rt-st) >= cps)
				break;
		}
		cpuTime = MPI_Wtime();
		//printf("CPU occupancy finished : %lf seconds \n", cpuTime);

		//调用MPI_Wait等待异步发送结束，记录时间
		MPI_Wait(&handle,&status);
		waitTime = MPI_Wtime();
		//printf("Sending task finished : %lf seconds \n", waitTime);

		totalTime = waitTime - startTime;
		printf("发送进程: 已经完成，用时 %.4lf 秒\n", totalTime);

	}else if(my_id == 0){

		startTime = MPI_Wtime();
		//阻塞接受
		MPI_Recv(buf, SIZE*SIZE, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
		//printf("Receiving task finished : %lf seconds \n", waitTime);
		waitTime = MPI_Wtime();
	}
	
	MPI_Finalize();
	return 0;
}
