#!/bin/bash
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 1
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 1
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 10
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 10
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 100
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 100
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 1000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 1000
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 10000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 10000
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 20000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 20000
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 40000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 40000
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 80000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 80000
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 100000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 100000
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 400000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 400000
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 1000000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 1000000
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 4000000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 4000000
wait
yhrun -p test -N 4 -n 4 ./mpi_halo_exchange 2 2 8000000
wait
yhrun -p test -N 4 -n 4 ./glex_halo_exchange 2 2 8000000