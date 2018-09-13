#!/bin/bash
for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 128
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 128
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 128
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 256
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 256
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 256
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 512
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 512
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 512
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 1024
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 1024
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 1024
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 2048
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 2048
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 2048
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 4096
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 4096
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 4096
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 8192
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 8192
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 8192
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 16384
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 16384
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 16384
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 32768
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 32768
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 32768
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 65536
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 65536
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 65536
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 131072
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 131072
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 131072
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 262144
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 262144
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 262144
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 524288
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 524288
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 524288
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 1048576
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 1048576
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 1048576
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 2097152
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 2097152
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 2097152
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 4194304
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 4194304
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 4194304
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 8388608
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 8388608
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 8388608
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 16777216
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 16777216
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 16777216
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 33554432
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 33554432
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 33554432
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 67108864
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 67108864
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 67108864
wait
done

for((i=1;i<=10;i++));
do
echo "Loop $i ——"
yhrun -p test -N 9 -n 9 ./mpi_halo_exchange 3 3 134217728
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_get 3 3 134217728
wait
yhrun -p test -N 9 -n 9 ./glex_halo_exchange_put 3 3 134217728
wait
done