#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <stddef.h>
#include <fcntl.h>
#include <signal.h>

#define BACKLOG 5     // how many pending connections queue will hold
#define BUF_SIZE 200
#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

int fd_A[BACKLOG];    // accepted connection fd
int conn_amount;    // current connection amount

void show_client()
{
    int i;
    printf("client amount: %d\n", conn_amount);
    for (i = 0; i < BACKLOG; i++) {
        printf("[%d]:%d  ", i, fd_A[i]);
    }
    printf("\n\n");
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, servlen, i, ret;
    socklen_t clilen;
    struct sockaddr_un cli_addr, serv_addr;
    char buf[BUF_SIZE];
    int yes = 1;
    fd_set fdsr;
    struct timeval tv;
    int maxsock;

    unlink("/tmp/test_serv");
    if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0) {
        perror("creating socket");
        exit(1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        close(sockfd);
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_LOCAL;
    servlen = snprintf(serv_addr.sun_path, sizeof serv_addr.sun_path,
            "%s", "/tmp/test_serv");
    servlen = offsetof(struct sockaddr_un, sun_path) + servlen;  

    if (bind(sockfd, (struct sockaddr *)&serv_addr, servlen) < 0) {
        perror("binding socket"); 
        close(sockfd);
        exit(1);
    }

    if (listen(sockfd, BACKLOG) < 0) {
        perror("listen socket"); 
        close(sockfd);
        exit(1);
    }

    conn_amount = 0;
    clilen = sizeof(cli_addr);
    maxsock = sockfd;
    while (1) {
        FD_ZERO(&fdsr);
        FD_SET(sockfd, &fdsr);

        tv.tv_sec = 30;
        tv.tv_usec = 0;

        for (i = 0; i < BACKLOG; i++) {
            if (fd_A[i] != 0) {
                FD_SET(fd_A[i], &fdsr);
            }
        }

        ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select");
            break;
        } else if (ret == 0) {
            printf("timeout\n");
            continue;
        }

        for (i = 0; i < conn_amount; i++) {
            if (FD_ISSET(fd_A[i], &fdsr)) {
                ret = recv(fd_A[i], buf, sizeof(buf), 0);
                if (ret <= 0) {        // client close
                    printf("client[%d] close\n", i);
                    close(fd_A[i]);
                    FD_CLR(fd_A[i], &fdsr);
                    fd_A[i] = 0;
                } else {        // receive data
                    if (ret < BUF_SIZE) {
                        memset(&buf[ret], '\0', 1);
                    }
                    printf("client[%d] send:%s\n", i, buf);
                    snprintf(buf, BUF_SIZE, "recieve %d bytes", ret);
                    send(fd_A[i], buf, strlen(buf), 0);
                }
            }
        }

        if (FD_ISSET(sockfd, &fdsr)) {
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd <= 0) {
                perror("accept");
                continue;
            }
            /* add to fd queue */
            if (conn_amount < BACKLOG) {
                fd_A[conn_amount++] = newsockfd;
                printf("new connection client[%d] %d\n", conn_amount, newsockfd);
                if (newsockfd > maxsock)
                    maxsock = newsockfd;
            }
            else {
                printf("max connections arrive, exit\n");
                send(newsockfd, "bye", 4, 0);
                close(newsockfd);
            }
        }

        show_client();
    }

    for (i = 0; i < BACKLOG; i++) {
        if (fd_A[i] != 0) {
            close(fd_A[i]);
        }
    }
    close(sockfd);
    exit(0);

#if 0
    clilen = sizeof(cli_addr);
    newsockfd = accept(
            sockfd,(struct sockaddr *)&cli_addr,&clilen);
    if (newsockfd < 0) 
        error("accepting");
    n=read(newsockfd,buf,80);
    printf("A connection has been established\n");
    write(1,buf,n);
    write(newsockfd,"I got your message\n",19);
    close(newsockfd);
    close(sockfd);
    return 0;
#endif
}
