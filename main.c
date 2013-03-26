#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h> 
#include <netdb.h> 
#include <stddef.h>

#include <cmd.h>

#define MYPORT 1234    // the port users will be connecting to
#define BACKLOG 5     // how many pending connections queue will hold
#define BUF_SIZE 200

int fd_A[BACKLOG];    // accepted connection fd
int conn_amount;    // current connection amount

void showclient()
{
    int i;
    printf("client amount: %d\n", conn_amount);
    for (i = 0; i < BACKLOG; i++) {
        printf("[%d]:%d  ", i, fd_A[i]);
    }
    printf("\n\n");
}


int d_running = 1;

struct systemcmd_d {
    int server_fd;
    int epfd;
#if 0
    struct epoll_event ev;
    struct epoll_event events[FD_SETSIZE];
#endif
};

struct systemcmd_d *daemon_create(void)
{
    struct systemcmd_d *daemon = NULL;

    daemon = calloc(1, sizeof(systemcmd_d));
    if (daemon == NULL) {
        perror("malloc");
        return NULL;
    }

    return daemon;

}

int bind_to_socket(struct systemcmd_d *daemon)
{
    struct sockaddr_un addr;
    socklen_t size, name_size;
    int fd_flags;
    int yes = 1;

    daemon->server_fd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (daemon->server_fd == -1) {
        perror("socket create");
        return -1;
    }

    if (setsockopt(daemon->server_fd, SOL_SOCKET, SO_REUSEADDR,
                &yes, sizeof(int)) < 0) {
        close(daemon->server_fd);
        perror("setsockopt");
        return -1;
    }

    addr.sun_family = AF_LOCAL;
    name_size = snprintf(addr.sun_path, sizeof addr.sun_path,
            "%s", SYSTEMCMD_SERVER);
    size = offsetof(struct sockaddr_un, sun_path) + name_size;  
    if (bind(daemon->server_fd, &addr, size) < 0) {
        close(daemon->server_fd);
        perror("bind");
        return -1;
    }

    printf("Debug: FD_SETSIZE=%d\n", FD_SETSIZE);
    if (listen(fd, FD_SETSIZE) < 0) {
        close(daemon->server_fd);
        perror("listen");
        return -1;
    }

    return 0;
}

int epoll_to_socket(struct systemcmd_d *daemon)
{
    struct epoll_event ev;

    daemon->epfd = epoll_create(FD_SETSIZE);

    if (daemon->epfd == -1) {
        perror("epoll_create");
        return -1;
    }

    ev.data.fd = daemon->server_fd;
    ev.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(daemon->epfd, EPOLL_CTL_ADD, ev.data.fd, &ev) < 0) {
        perror("epoll_ctl");
        return -1;
    }

    return 0;
}

int daemon_init(struct systemcmd_d *daemon)
{
    if (bind_to_socket(daemon) < 0)
        return -1;

    if (epoll_to_socket(daemon) < 0)
        return -1;

    return 0;
}

void daemon_destroy(struct systemcmd_d *daemon)
{
    if (daemon->server_fd)
        close(daemon->server_fd);

    if (daemon->epfd)
        close(daemon->epfd);

    if (daemon)
        free(daemon);

    unlink(SYSTEMCMD_SERVER);
}

void daemon_run(struct systemcmd_d *daemon)
{
    while (d_running) {
        struct epoll_event ev;
        int n;

        n = epoll_wait(daemon->epfd, &ev, 1, -1);

        if (n < 0)
            error(0, errno, "epoll_wait failed");
        if (n != 1)
            continue;

    }
}

int main(int argc, const char *argv[])
{
    struct systemcmd_d *daemon = NULL;
    daemon = daemon_create();
    if (daemon == NULL)
        goto out;

    if (daemon_init(daemon) < 0)
        goto destroy;

    daemon_run(daemon);
    
destroy:
    daemon_destroy(daemon);

out:
    return 0;
}

int main(void)
{
    int sock_fd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_in server_addr;    // server address information
    struct sockaddr_in client_addr; // connector's address information
    socklen_t sin_size;
    int yes = 1;
    char buf[BUF_SIZE];
    int ret;
    int i;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;         // host byte order
    server_addr.sin_port = htons(MYPORT);     // short, network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sock_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("listen port %d\n", MYPORT);

    fd_set fdsr;
    int maxsock;
    struct timeval tv;

    conn_amount = 0;
    sin_size = sizeof(client_addr);
    maxsock = sock_fd;
    while (1) {

        // initialize file descriptor set
        FD_ZERO(&fdsr);
        FD_SET(sock_fd, &fdsr);

        // timeout setting
        tv.tv_sec = 30;
        tv.tv_usec = 0;

        // add active connection to fd set
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

        // check every fd in the set
        for (i = 0; i < conn_amount; i++) {
            if (FD_ISSET(fd_A[i], &fdsr)) {
                ret = recv(fd_A[i], buf, sizeof(buf), 0);
                if (ret <= 0) {        // client close
                    printf("client[%d] close\n", i);
                    close(fd_A[i]);
                    FD_CLR(fd_A[i], &fdsr);
                    fd_A[i] = 0;
                } else {        // receive data
                    if (ret < BUF_SIZE)
                        memset(&buf[ret], '\0', 1);
                    printf("client[%d] send:%s\n", i, buf);
                }
            }
        }

        // check whether a new connection comes
        if (FD_ISSET(sock_fd, &fdsr)) {
            new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size);
            if (new_fd <= 0) {
                perror("accept");
                continue;
            }
            // add to fd queue
            if (conn_amount < BACKLOG) {
                fd_A[conn_amount++] = new_fd;
                printf("new connection client[%d] %s:%d\n", conn_amount,
                        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                if (new_fd > maxsock)
                    maxsock = new_fd;
            }
            else {
                printf("max connections arrive, exit\n");
                send(new_fd, "bye", 4, 0);
                close(new_fd);
                break;
            }
        }
        showclient();
    }

    // close other connections
    for (i = 0; i < BACKLOG; i++) {
        if (fd_A[i] != 0) {
            close(fd_A[i]);
        }
    }

    exit(0);

}
