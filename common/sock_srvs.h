


#ifndef __SOCK_SRVS_H__
#define __SOCK_SRVS_H__

#define SOCK_FD_STATE_CLOSE     0
#define SOCK_FD_STATE_INIT      1
#define SOCK_FD_STATE_CONNECT   2
#define SOCK_FD_STATE_ACCEPT    3

#define SOCK_SRV_FLAG_RESTART    1

#define SOCK_MAX_CLT_NUM    64

typedef struct STRUCT_CLT_SOCK_
{
    int fd;
    unsigned int   cip;
    unsigned char *recv_data;
    unsigned short cport;
    unsigned short recv_len;
    unsigned short remain_len;
}STRUCT_CLT_SOCK;


typedef struct STRUCT_SRV_SOCK_ELMT_
{
    int fd;
    char ip[16];
    unsigned short port;
    unsigned short state;
    unsigned short retry_interval;
    unsigned short counter;
    unsigned short flag;
    unsigned short clt_num;
    STRUCT_CLT_SOCK clts[SOCK_MAX_CLT_NUM];
    int (*init)(unsigned short clt_id);
    int (*recv)(unsigned short clt_id);
    int (*send)(unsigned short clt_id);
}STRUCT_SRV_SOCK_ELMT;


extern STRUCT_SRV_SOCK_ELMT g_srv_sock_elmt[];
extern unsigned char g_srv_sock_elmt_num;
extern unsigned char g_srv_restart;

void *sock_servers_proc(__attribute__((unused))void *arg);

#endif

