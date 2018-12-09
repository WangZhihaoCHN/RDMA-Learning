#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "math.h"
#include "mpi.h"

float create_rand_nums() {
  float rand_nums = (rand() / (float)RAND_MAX);
  return rand_nums;
}

int main(int argc,char** argv)
{
    int          taskid, ntasks;
    MPI_Status   status;
    MPI_Request  handle;
    float        local, global_sum, global_max, global_min;
    /* MPI初始化 */
    MPI_Init(&argc, &argv);

    /* 获得MPI进程数量以及进程编号 */
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Comm_size(MPI_COMM_WORLD,&ntasks);
    
    local = 0.1 + (float)taskid;
    //printf("rank %d——%f  ", taskid, local);

    MPI_Allreduce(&local, &global_sum, 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&local, &global_max, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&local, &global_min, 1, MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD); 
    
    if (taskid == 0){
        printf("-*- Tree-based Algorithm Results -*-\n");
        printf("Sum:%f\t", global_sum);
        printf("Max:%f\t", global_max);
        printf("Min:%f\n", global_min);
    }

    /* MPI程序结束 */
    MPI_Finalize();
}