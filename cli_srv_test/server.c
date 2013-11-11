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
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "common.h"

#define BACKLOG 1   /* Not very useful on linux */
#define BUF_SIZE 200
#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

int cli_fd[100];    // accepted connection fd
int conn_amount;    // current connection amount

void show_client()
{
    int i;
    loglog("client amount: %d\n", conn_amount);
    for (i = 0; i < conn_amount; i++) {
        fprintf(stderr, "[%d]:%d  ", i, cli_fd[i]);
    }
    fprintf(stderr, "\n\n");
}

static int log_timestamp(void)
{
    struct timeval tv;
    struct tm *brokendown_time;
    char string[128];

    gettimeofday(&tv, NULL);

    brokendown_time = localtime(&tv.tv_sec);
    strftime(string, sizeof string, "%H:%M:%S", brokendown_time);

    return fprintf(stderr, "[%s.%03li] ", string, tv.tv_usec/1000);
}

SRV_EXPORT void loglog(const char *fmt, ...)
{
    va_list argp;

    va_start(argp, fmt);                                                                                                                                                                                       
    log_timestamp();
    vfprintf(stderr, fmt, argp);
    va_end(argp);
}

int bind_to_socket()
{
    int sockfd = -1;
    int servlen = 0;
    int yes = 1;
    struct sockaddr_un serv_addr;

    unlink(SERVER_SOCK_PATH);
    if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0) {
        loglog("error creating socket: %s\n", strerror(errno));
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
                &yes, sizeof(int)) == -1) {
        loglog("error setsockopt: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_LOCAL;
    servlen = snprintf(serv_addr.sun_path, sizeof serv_addr.sun_path,
            "%s", SERVER_SOCK_PATH);
    servlen = offsetof(struct sockaddr_un, sun_path) + servlen;  

    if (bind(sockfd, (struct sockaddr *)&serv_addr, servlen) < 0) {
        loglog("error binding socket: %s\n", strerror(errno)); 
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, BACKLOG) < 0) {
        loglog("error listen socket: %s\n", strerror(errno)); 
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, i, ret;
    struct sockaddr_un cli_addr;
    socklen_t clilen;
    char buf[BUF_SIZE];
    fd_set fdsr;
    struct timeval tv;
    int maxsock;

    sockfd = bind_to_socket();
    if (sockfd < 0)
        exit(-1);

    conn_amount = 0;
    clilen = sizeof(cli_addr);
    maxsock = sockfd;

    loglog("Server Start...\n");
    while (1) {
        FD_ZERO(&fdsr);
        FD_SET(sockfd, &fdsr);

        tv.tv_sec = 30;
        tv.tv_usec = 0;

        for (i = 0; i < SERVER_MAX_CLIENT; i++) {
            if (cli_fd[i] != 0) {
                FD_SET(cli_fd[i], &fdsr);
            }
        }

        ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
        if (ret < 0) {
            loglog("error select: %s\n", strerror(errno));
            break;
        } else if (ret == 0) {
            loglog("timeout\n");
            continue;
        }

        /* Check previous clients */
        for (i = 0; i < conn_amount; i++) {
            if (FD_ISSET(cli_fd[i], &fdsr)) {
                ret = recv(cli_fd[i], buf, sizeof(buf), 0);
                if (ret <= 0) {        // client close
                    loglog("client[%d] close\n", i);
                    close(cli_fd[i]);
                    shutdown(cli_fd[i], SHUT_RDWR);
                    FD_CLR(cli_fd[i], &fdsr);
                    cli_fd[i] = 0;
                } else {        // receive data
                    if (ret < BUF_SIZE) {
                        memset(&buf[ret], '\0', 1);
                    }
                    loglog("client[%d] send:%s\n", i, buf);
                    snprintf(buf, BUF_SIZE, "recieve %d bytes", ret);
                    send(cli_fd[i], buf, strlen(buf), 0);
                }
            }
        }

        if (FD_ISSET(sockfd, &fdsr)) {
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd <= 0) {
                loglog("error accept: %s\n", strerror(errno));
                continue;
            }
            /* add to fd queue */
            if (conn_amount < 100) {
                cli_fd[conn_amount++] = newsockfd;
                loglog("new connection client[%d] %d\n", conn_amount, newsockfd);
                if (newsockfd > maxsock)
                    maxsock = newsockfd;
            }
            else {
                loglog("max connections arrive, exit\n");
                send(newsockfd, "bye", 4, 0);
                close(newsockfd);
                shutdown(newsockfd, SHUT_RDWR);
            }
        }

        show_client();
    }

    for (i = 0; i < BACKLOG; i++) {
        if (cli_fd[i] != 0) {
            close(cli_fd[i]);
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
    loglog("A connection has been established\n");
    write(1,buf,n);
    write(newsockfd,"I got your message\n",19);
    close(newsockfd);
    close(sockfd);
    return 0;
#endif
}
