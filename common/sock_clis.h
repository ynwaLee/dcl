#ifndef __SOCK_CLIS_H__
#define __SOCK_CLIS_H__

#define SOCK_FD_STATE_CLOSE     0
#define SOCK_FD_STATE_INIT      1
#define SOCK_FD_STATE_CONNECT   2

#define SOCK_SRV_FLAG_RECONN    1

typedef struct STRUCT_SERVER_ELMT_
{
    int fd;
    char ip[16];
    unsigned short port;
    unsigned short state;
    unsigned short retry_interval;
    unsigned short counter;
    unsigned short flag;
    unsigned short recv_len;
    unsigned short remain_len;
    unsigned char *recv_data;
    int (*init)(int fd);
    int (*recv)(int fd);
    int (*send)(int fd);
}STRUCT_SERVER_ELMT;


extern STRUCT_SERVER_ELMT g_server_elmt[];
extern unsigned char g_server_elmt_num;
extern unsigned char g_server_reconnect;

void *sock_server_proc(__attribute__((unused))void *arg);
void *sock_clients_proc(__attribute__((unused))void *arg);

#endif
