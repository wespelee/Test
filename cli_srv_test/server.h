#ifndef SERVER_H
#define SERVER_H

struct cli_info {
    struct hlist link;
    /* Client socket */
    int cli_fd;
};

struct srv_info {
    /* Server local socket */
    int sock;
    /* Client list head */
    struct hlist cli_list;
    int total_client;

    /* Use for select() */
    int max_sock;
    fd_set fdsr;
};

#endif
