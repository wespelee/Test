all: server.c client.c common.h
	gcc -g -Wall -I./ -o server server.c hlist.c
	gcc -g -Wall -I./ -o client client.c

clean:
	rm *.o

