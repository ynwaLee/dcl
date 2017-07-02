#ifndef __DCL_ONLINE_H__
#define __DCL_ONLINE_H__


#define MAX_CLUE_NUM    1024
#define MAX_BUF_SIZE        20480

typedef struct obj_clue_node_ {
    unsigned int obj;
    unsigned int clue;
} obj_clue_node_t;

typedef struct obj_clue_ {
    obj_clue_node_t node[MAX_CLUE_NUM];
    unsigned int counter;
} obj_clue_t;

typedef struct sock_buf_node_ {
    char recv_buf[MAX_BUF_SIZE];
    unsigned int recv_len;
    unsigned int remain_len;
} sock_buf_node_t;


void *thread_dcl_online(void *arg);
int create_tcp_server(char *ip, uint16_t port);


#endif
