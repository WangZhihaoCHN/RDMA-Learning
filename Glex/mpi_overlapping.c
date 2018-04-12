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
	char buf[SIZE][SIZE];		//发送缓冲区
	MPI_Status status;			//status对象(Status)
	MPI_Request handle;			//MPI请求(request)

	//记录MPI_Isend()开始时间，cpu占用结束时间，MPI_Wait()结束时间
	double startTime,cpuTime,waitTime,totalTime;		

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&num_procs);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_id);

	//两个进程，进程1非阻塞发送，进程0阻塞接收
	if(my_id == 1){
		// 同步
		MPI_Barrier(MPI_COMM_WORLD); 

		// 开始发送，计时 
		startTime = MPI_Wtime();

		// MPI异步发送
		MPI_Isend(buf, SIZE*SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &handle);
		printf("发送进程: 开始发送，占用CPU %.2lf个单位时间\n", SEC);

		/*clock_t st, rt;				//占用cpu开始时间，占用cpu当前执行时间
		st = clock();
		double cps = (double)(SEC * CLOCKS_PER_SEC);  //需要占用SEC秒数
		while(1){
			rt = clock();
			if( (double)(rt-st) >= cps)
				break;
		}*/

		//占用CPU一段时间（SEC单位时间）
		volatile double timer = 0;
		int i=0;
		for(i=0; i<SEC*10000000; i++)
			timer += i*i*2;
		
		cpuTime = MPI_Wtime();
		printf("发送进程: CPU占用完毕，用时 %lf 秒\n", cpuTime-startTime);

		//调用MPI_Wait等待异步发送结束，记录时间
		MPI_Wait(&handle,&status);

		// 同步
		MPI_Barrier(MPI_COMM_WORLD); 

		endTime = MPI_Wtime();
		//printf("Sending task finished : %lf seconds \n", waitTime);

		totalTime = endTime - startTime;
		printf("发送进程: 已经完成，用时 %.4lf 秒\n", totalTime);

	}else if(my_id == 0){
		// 同步
		MPI_Barrier(MPI_COMM_WORLD); 

		//阻塞接受
		MPI_Recv(buf, SIZE*SIZE, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status);

		// 同步
		MPI_Barrier(MPI_COMM_WORLD); 
	}
	
	MPI_Finalize();
	return 0;
}
