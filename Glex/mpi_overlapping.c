#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"

int main(int argc, char *argv[])
{
	double SEC = 0;
	int SIZE = 0;
	if(argc == 3){
		SIZE = atoi(argv[1]);
		SEC = atof(argv[2]);
	}else{
		printf("输入参数错误！需要两个参数： 1.二维char数组维数; 2.cpu halt时长");
		return -1;
	}

	printf("发送数据为：%d字节\n", SIZE*SIZE);
	int num_procs,my_id,flag;	//进程总数、进程ID、传输是否完成标识
	
	//发送缓冲区
	char **buf = (char**)malloc(SIZE*sizeof(char*));	
	int memTmp = 0;
	for(memTmp=0;memTmp<SIZE;++memTmp)
		buf[memTmp] = (char*)malloc(SIZE*sizeof(char));
	memset(buf, 65, SIZE*SIZE);

	MPI_Status status;			//status对象(Status)
	MPI_Request handle;			//MPI请求(request)

	//记录MPI_Isend()开始时间，cpu占用结束时间，MPI_Wait()结束时间
	double startTime,cpuTime,endTime,totalTime;		

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&num_procs);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_id);

	// 同步
	MPI_Barrier(MPI_COMM_WORLD); 
	// 开始计时 
	startTime = MPI_Wtime();
	//两个进程，进程1非阻塞发送，进程0阻塞接收
	if(my_id == 0){
		memset(buf, 66, SIZE*SIZE);
		// MPI异步发送
		MPI_Isend(buf, SIZE*SIZE, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &handle);

		//占用CPU一段时间（SEC单位时间）
		volatile double timer = 0;
		long i=0;
		for(i=0; i<SEC*1000; i++)
			timer += i*i*2;

		//调用MPI_Wait等待异步发送结束，记录时间
		MPI_Wait(&handle,&status);
	}else{
		//阻塞接受
		MPI_Recv(buf, SIZE*SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
	}
	// 同步
	MPI_Barrier(MPI_COMM_WORLD); 
	// 结束计时
	endTime = MPI_Wtime();
	totalTime = endTime - startTime;
	printf("数据块发送已经完成，用时 %.4lf ms\n", totalTime*1000);

	MPI_Finalize();

	// 释放内存
	// for(memTmp=0; memTmp<SIZE; memTmp++)
	// 	free(buf[memTmp]);
	free(buf);
	
	return 0;
}
