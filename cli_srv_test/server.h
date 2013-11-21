#ifndef SERVER_H
#define SERVER_H

struct client {
    struct hlist link;
    /* Client socket */
    int cli_fd;
};

struct srv_info {
    /* Server local socket */
    int sock;
    /* Client list head */
    struct hlist client_list;

    /* Use for select() */
    int max_sock;
    fd_set fdsr;
};

#endif
