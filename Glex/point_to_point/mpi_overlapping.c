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

	printf("发送数据为：%dM字节\n", SIZE);
	SIZE *= (1000*1000);
	int num_procs,my_id,flag;	//进程总数、进程ID、传输是否完成标识
	
	//发送缓冲区
	char *buf = (char*)malloc(SIZE*sizeof(char));
	int i;
    for(i=0;i<SIZE;i++){
        buf[i] = rand()%256;
    }

	MPI_Status status;			//status对象(Status)
	MPI_Request handle;			//MPI请求(request)

	//记录MPI_Isend()开始时间，cpu占用结束时间，MPI_Wait()结束时间
	double startTime,cpuTime,endTime,totalTime;		

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD,&num_procs);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_id);

	// 开始计时 
	int loop = 0;
    totalTime = 0;
    MPI_Barrier(MPI_COMM_WORLD); 
	startTime = MPI_Wtime();

	for(loop=0;loop<300;loop++){
        //两个进程，进程1非阻塞发送，进程0阻塞接收
		if(my_id == 0){
			//memset(buf, 66, SIZE);
			// MPI异步发送
			MPI_Isend(buf, SIZE, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &handle);

			//占用CPU一段时间（SEC单位时间）
			volatile double timer = 0;
			long i=0;
			for(i=0; i<SEC*1000*100; i++)
				timer += i*i*2;

			//调用MPI_Wait等待异步发送结束，记录时间
			MPI_Wait(&handle,&status);
		}else{
			//阻塞接受
			MPI_Recv(buf, SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
		}
		/* 等待通信结束，计时 */
		MPI_Barrier(MPI_COMM_WORLD);
	}
	if(my_id == 0){
		endTime = MPI_Wtime();
		totalTime = (endTime - startTime) * 1e6 / 300 / 1000;
		printf("MPI_P2P: Trans %d Bytes,",SIZE);
		printf("totaltime %.5lf ms\n", totalTime);
	}

	MPI_Finalize();
	free(buf);
	
	return 0;
}
