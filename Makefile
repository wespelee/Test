pthread_test: pthread_test.c
	gcc -g -o pthread_test pthread_test.c -lpthread	

cmd_server: main.c cmd.h
	gcc -g -I. -Wall -o test main.c
	valgrind -v --leak-check=yes ./test

gcc -g -pthread -finstrument-functions -o r_test r_test.c
