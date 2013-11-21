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
#include <time.h>
#include <sys/time.h>

#define BUF_SIZE 200

int main(int argc, char *argv[])
{
    int sockfd, servlen, n;
    struct sockaddr_un  serv_addr;
    char buffer[BUF_SIZE];
    pid_t my_pid = getpid();

    struct tm *brokendown_time;
    struct timeval tv;
    char string[128];

    bzero((char *)&serv_addr,sizeof(serv_addr));
    serv_addr.sun_family = AF_LOCAL;
    servlen = snprintf(serv_addr.sun_path, sizeof serv_addr.sun_path,
            "%s", "/tmp/test_serv");
    servlen = offsetof(struct sockaddr_un, sun_path) + servlen;  

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Creating socket");
        exit(1);
    }
    if (connect(sockfd, (struct sockaddr *) 
                &serv_addr, servlen) < 0) {
        perror("Connecting");
        close(sockfd);
        exit(1);
    }

    bzero(buffer, BUF_SIZE);
    gettimeofday(&tv, NULL);
    brokendown_time = localtime(&tv.tv_sec);
    strftime(string, sizeof string, "%H:%M:%S", brokendown_time);

    snprintf(buffer, BUF_SIZE, "%s PID: %d send", string, my_pid);
    printf("My pid = %d is ready to send\n", my_pid);
    write(sockfd, buffer, strlen(buffer));
    printf("My pid = %d is ready to read\n", my_pid);
    fflush(NULL);
    n = read(sockfd, buffer, BUF_SIZE);
    buffer[n] = '\0';
    printf("%d The return message was: %s\n", my_pid, buffer);
    fflush(NULL);
    sleep(10);
    close(sockfd);
    return 0;
}
