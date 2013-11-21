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
#include "hlist.h"
#include "server.h"

#define MAX_CLIENT (10)
#define BACKLOG 1   /* Not very useful on linux */
#define BUF_SIZE 200
#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

void show_client(struct srv_info *srv)
{
    struct client *cli;
    int i = 0;

    loglog("client amount: %d\n", hlist_length(&srv->client_list));

    hlist_for_each(cli, &srv->client_list, link) {
        loglog("[%d]:%d\n", i, cli->cli_fd);
        i++;
    }
}

static int make_socket_non_blocking (int sfd)
{
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if (s == -1) {
        perror("fcntl");
        return -1;
    }

    return 0;
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

void client_destroy(struct client *client)
{
    if (client) {
        hlist_remove(&client->link);
        close(client->cli_fd);
        shutdown(client->cli_fd, SHUT_RDWR);
        free(client);
        client = NULL;
    }
    return;
}

struct client *create_client(int fd)
{
    struct client *client = NULL;

    client = malloc(sizeof(*client));                                                                                                                                                                          
    if (client == NULL) {
        loglog("error client malloc:%s\n", strerror(errno));
        return NULL;
    }

    memset(client, 0, sizeof(*client));
    hlist_init(&client->link);

    client->cli_fd = fd;

    return client;
}

void server_full_quit(int fd)
{
    if (fd) {
        close(fd);
        shutdown(fd, SHUT_RDWR);
    }
    return;
}

void add_client(struct srv_info *srv)
{
    int cli_sock;
    struct client *client = NULL;
    struct sockaddr_un cli_addr;
    socklen_t cli_len = 0;
    int len = 0;

    memset(&cli_addr, 0, sizeof(cli_addr));
    len = hlist_length(&srv->client_list);

    cli_sock = accept(srv->sock, (struct sockaddr *)&cli_addr, &cli_len);
    if (cli_sock <= 0) {
        loglog("error accept: %s\n", strerror(errno));
        return;
    }

    if (len > MAX_CLIENT) {
        loglog("max connections arrive!\n");
        server_full_quit(cli_sock);
        return;
    }

    client = create_client(cli_sock);

    if (client) {
        hlist_insert(&srv->client_list, &client->link);
    }

    return;
}

int server_init(struct srv_info *srv)
{
    memset(srv, 0, sizeof(struct srv_info));
    hlist_init(&srv->client_list);

    srv->sock = bind_to_socket();

    if (srv->sock < 0)
        return -1;

    srv->max_sock = srv->sock;

    return 0;
}

void select_fd_init(struct srv_info *srv)
{
    struct client *cli;
    int max_fd = srv->sock;

    FD_ZERO(&srv->fdsr);
    FD_SET(srv->sock, &srv->fdsr);

    hlist_for_each(cli, &srv->client_list, link) {
        loglog("client fd:%d\n", cli->cli_fd);
        FD_SET(cli->cli_fd, &srv->fdsr);

        if (cli->cli_fd > max_fd)
            max_fd = cli->cli_fd;
    }

    srv->max_sock = max_fd;
    loglog("max sock:%d\n", srv->max_sock);
}

void handle_recv_client(struct client *client)
{
    char buf[BUF_SIZE];
    int ret;

    ret = recv(client->cli_fd, buf, sizeof(buf), 0);
    if (ret == 0) {
        /* Client close */
        loglog("client %d close: %d err:%s\n", client->cli_fd, ret, strerror(errno));
        client_destroy(client);
    }
    else if (ret < 0 && errno == EAGAIN) {
        loglog("client %d needs again: %d err:%s\n", client->cli_fd, ret, strerror(errno));
    }
    else {
        /* Receive data */
        if (ret < BUF_SIZE) {
            buf[ret] = '\0';
            loglog("client[%d] send:%s\n", client->cli_fd, buf);
            snprintf(buf, BUF_SIZE, "server recieve %d bytes", ret);
            send(client->cli_fd, buf, strlen(buf), 0);
        }
        else {
            loglog("server recieve %d buffer overflow\n", client->cli_fd);
            client_destroy(client);
        }
    }
}

void process_client(struct srv_info *srv)
{
    struct client *cli, *next;

    hlist_for_each_safe(cli, next, &srv->client_list, link) {
        if (FD_ISSET(cli->cli_fd, &srv->fdsr))
            handle_recv_client(cli);
    }
}

void server_close(struct srv_info *srv)
{
    struct client *cli, *next;

    hlist_for_each_safe(cli, next, &srv->client_list, link)
        client_destroy(cli);

    close(srv->sock);
}

int main(int argc, char *argv[])
{
    int ret;
    struct timeval tv;
    int srv_run = 1;

    struct srv_info srv;

    if (server_init(&srv) < 0)
        exit(-1);

    loglog("Server Start...\n");
    while (srv_run) {
        tv.tv_sec = 30;
        tv.tv_usec = 0;

        select_fd_init(&srv);

        //ret = select(srv.max_sock + 1, &srv.fdsr, NULL, NULL, &tv);
        ret = select(srv.max_sock + 1, &srv.fdsr, NULL, NULL, &tv);
        if (ret < 0) {
            loglog("error select: %s\n", strerror(errno));
            break;
        } else if (ret == 0) {
            loglog("timeout\n");
            srv_run = 0;
            continue;
        }

        loglog("Something happened...\n");

        process_client(&srv);

        if (FD_ISSET(srv.sock, &srv.fdsr)) {
            add_client(&srv);
        }

        show_client(&srv);
    }

    loglog("Server Stop...\n");
    server_close(&srv);
    exit(0);
}
