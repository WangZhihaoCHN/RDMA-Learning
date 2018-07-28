#!/bin/bash
mv Config_480 Config
mv TM_Mapping_Perm_480 TM_Mapping_Perm
yhrun -N 20 -n 480 ./pingpong
wait
yhrun -N 20 -n 480 ./tea_leaf
wait
yhrun -N 20 -n 480 ./tea_leaf
wait
yhrun -N 20 -n 480 ./tea_leaf
wait
yhrun -N 20 -n 480 ./tea_leaf
wait
yhrun -N 20 -n 480 ./tea_leaf
wait
yhrun -N 1 -n 1  ./RCR
wait
yhrun -N 20 -n 480  -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait
yhrun -N 20 -n 480  -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait
yhrun -N 20 -n 480  -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait
yhrun -N 20 -n 480  -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait
yhrun -N 20 -n 480  -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait
yhrun -N 1 -n 1 ./TM_Mapping
wait
yhrun -N 20 -n 480  -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait
yhrun -N 20 -n 480  -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait
yhrun -N 20 -n 480  -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait
yhrun -N 20 -n 480  -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait
yhrun -N 20 -n 480  -m arbitrary -w ./TM_Mapping_Result ./tea_leaf

mv Config_512 Config
mv TM_Mapping_Perm_512 TM_Mapping_Perm

yhrun -N 22 -n 512 ./pingpong 
wait
yhrun -N 22 -n 512 ./tea_leaf
wait               
yhrun -N 22 -n 512 ./tea_leaf
wait               
yhrun -N 22 -n 512 ./tea_leaf
wait               
yhrun -N 22 -n 512 ./tea_leaf
wait               
yhrun -N 22 -n 512 ./tea_leaf
wait
yhrun -N 1 -n 1  ./RCR
wait
yhrun -N 22 -n 512 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait               
yhrun -N 22 -n 512 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait               
yhrun -N 22 -n 512 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait               
yhrun -N 22 -n 512 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait               
yhrun -N 22 -n 512 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait
yhrun -N 1 -n 1 ./TM_Mapping
wait
yhrun -N 22 -n 512 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait               
yhrun -N 22 -n 512 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait               
yhrun -N 22 -n 512 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait               
yhrun -N 22 -n 512 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait               
yhrun -N 22 -n 512 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait

mv Config_640 Config
mv TM_Mapping_Perm_640 TM_Mapping_Perm

yhrun -N 27 -n 640 ./pingpong
wait
yhrun -N 27 -n 640 ./tea_leaf
wait               
yhrun -N 27 -n 640 ./tea_leaf
wait               
yhrun -N 27 -n 640 ./tea_leaf
wait               
yhrun -N 27 -n 640 ./tea_leaf
wait               
yhrun -N 27 -n 640 ./tea_leaf
wait
yhrun -N 1 -n 1  ./RCR
wait
yhrun -N 27 -n 640 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait               
yhrun -N 27 -n 640 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait               
yhrun -N 27 -n 640 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait               
yhrun -N 27 -n 640 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait               
yhrun -N 27 -n 640 -m arbitrary -w ./RCR_Mapping_Result ./tea_leaf
wait
yhrun -N 1 -n 1 ./TM_Mapping
wait
yhrun -N 27 -n 640 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait               
yhrun -N 27 -n 640 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait               
yhrun -N 27 -n 640 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait               
yhrun -N 27 -n 640 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
wait               
yhrun -N 27 -n 640 -m arbitrary -w ./TM_Mapping_Result ./tea_leaf
