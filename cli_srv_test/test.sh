#!/bin/sh

valgrind --leak-check=full -v ./server 2>&1 | tee server.log & 

sleep 5
for i in `seq 1 100`; 
do 
    ./client &
done
