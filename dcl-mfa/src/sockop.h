#ifndef __SOCKOP_H__
#define __SOCKOP_H__

#include <stdbool.h>

int sock_recv(int sockfd, void *buf, int size, int *recved);
int sock_send(int sockfd, void *buf, int size, int *sended);

int sock_connect(const char *ip, unsigned short port);
int sock_listen(const char *ip, unsigned short port);

bool sock_readable(int sockfd, int sec, int usec);
#endif
