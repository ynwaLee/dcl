


#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "applog.h"
#include "file_func.h"
#include "sock_srvs.h"
#include "thread_flag.h"







#define SOCK_SRV_MAX_LISTEN    16

static int start_server(char *ip, short port)
{
    int tcp_server_fd;
    struct sockaddr_in serveraddr;
    int yes = 1;

    tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == tcp_server_fd)
    {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "start_server: fail to creat socket");
        return -1;
    }

    if (setsockopt(tcp_server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "start_server: fail to setsockopt");
        return -1;
    }

    memset(&serveraddr , 0, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    if (bind(tcp_server_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
    {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "start_server: bind %s:%u fail", ip, port);
        return -1;
    }

    if (listen(tcp_server_fd, SOCK_SRV_MAX_LISTEN) == -1)
    {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "start_server: socket listen error");
        return -1;
    }

    return tcp_server_fd;
}


STRUCT_SRV_SOCK_ELMT g_srv_sock_elmt[8];
unsigned char g_srv_sock_elmt_num = 0;
unsigned char g_srv_restart = THREAD_OPR_FLAG_IDLE;

void *sock_servers_proc(__attribute__((unused))void *arg)
{
    int  ret, fd;
     int i, j;
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);

    while (1)
    {
        if (svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))
            return NULL;

        if (g_srv_restart == THREAD_OPR_FLAG_REQ)
        {
            for(i = 0; i < g_srv_sock_elmt_num; i++)
            {
                if (g_srv_sock_elmt[i].flag == SOCK_SRV_FLAG_RESTART)
                {
                    for (j = 0; j < SOCK_MAX_CLT_NUM; j++)
                    {
                        if (g_srv_sock_elmt[i].clts[j].fd)
                        {
                            close(g_srv_sock_elmt[i].clts[j].fd);
                            g_srv_sock_elmt[i].clts[j].fd = 0;
                        }
                    }
                    close(g_srv_sock_elmt[i].fd);
                    g_srv_sock_elmt[i].clt_num = 0;
                    g_srv_sock_elmt[i].state = SOCK_FD_STATE_CLOSE;
                    g_srv_sock_elmt[i].flag = 0;
                }
            }
        }

        for(i = 0; i < g_srv_sock_elmt_num; i++)
        {
            if (g_srv_sock_elmt[i].state == SOCK_FD_STATE_CLOSE)
            {
                if (g_srv_sock_elmt[i].counter == 0)
                {
                    if ((g_srv_sock_elmt[i].fd = start_server(g_srv_sock_elmt[i].ip, g_srv_sock_elmt[i].port)) == -1)
                    {
                        g_srv_sock_elmt[i].counter = g_srv_sock_elmt[i].retry_interval;
                        usleep(1000);
                    }
                    else
                    {
                        g_srv_sock_elmt[i].state = SOCK_FD_STATE_ACCEPT;
                    }
                }
                else
                {
                    g_srv_sock_elmt[i].counter--;
                    usleep(200);
                }
            }
        }
        clear_file_set();
        for (i = 0; i < g_srv_sock_elmt_num; i++)
        {
            if (g_srv_sock_elmt[i].state == SOCK_FD_STATE_ACCEPT)
            {
                check_file_add_fd(g_srv_sock_elmt[i].fd);
                for (j = 0; j < SOCK_MAX_CLT_NUM; j++)
                {
                    if (g_srv_sock_elmt[i].clts[j].fd)
                        check_file_add_fd(g_srv_sock_elmt[i].clts[j].fd);
                }
            }
        }
        ret = check_files_readable();
        if (ret > 0)
        {
            for (i = 0; i < g_srv_sock_elmt_num; i++)
            {
                if (g_srv_sock_elmt[i].state == SOCK_FD_STATE_ACCEPT)
                {
                    for (j = 0; j < SOCK_MAX_CLT_NUM; j++)
                    {
                        if ((g_srv_sock_elmt[i].clts[j].fd)&&(file_readable(g_srv_sock_elmt[i].clts[j].fd)))
                        {
                            if (g_srv_sock_elmt[i].clts[j].remain_len == 0)
                            {
                                ret = g_srv_sock_elmt[i].recv(j);
                                if (ret)
                                {
                                    close(g_srv_sock_elmt[i].clts[j].fd);
                                    g_srv_sock_elmt[i].clts[j].fd = 0;
                                    g_srv_sock_elmt[i].clt_num--;
                                }
                            }
                            else
                            {
                                ret = recv(g_srv_sock_elmt[i].clts[j].fd,
                                    &g_srv_sock_elmt[i].clts[j].recv_data[g_srv_sock_elmt[i].clts[j].recv_len],
                                    g_srv_sock_elmt[i].clts[j].remain_len, 0);
                                if (ret <= 0)
                                {
                                    close(g_srv_sock_elmt[i].clts[j].fd);
                                    g_srv_sock_elmt[i].clts[j].fd = 0;
                                    g_srv_sock_elmt[i].clt_num--;
                                }
                                else
                                {
                                    g_srv_sock_elmt[i].clts[j].remain_len -= ret;
                                    g_srv_sock_elmt[i].clts[j].recv_len += ret;
                                    if (g_srv_sock_elmt[i].clts[j].remain_len == 0)
                                    {
                                        ret = g_srv_sock_elmt[i].recv(j);
                                        if (ret)
                                        {
                                            close(g_srv_sock_elmt[i].clts[j].fd);
                                            g_srv_sock_elmt[i].clts[j].fd = 0;
                                            g_srv_sock_elmt[i].clt_num--;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (file_readable(g_srv_sock_elmt[i].fd))
                    {
                        fd = accept(g_srv_sock_elmt[i].fd, (struct sockaddr *)&clientaddr, &addrlen);
                        if (fd <= 0)
                        {
                            continue;
                        }
                        if (g_srv_sock_elmt[i].clt_num < SOCK_MAX_CLT_NUM)
                        {
                            for (j = 0; j < SOCK_MAX_CLT_NUM; j++)
                            {
                                if (g_srv_sock_elmt[i].clts[j].fd == 0)
                                {
                                    g_srv_sock_elmt[i].clts[j].fd = fd;
                                    g_srv_sock_elmt[i].clts[j].cip = clientaddr.sin_addr.s_addr;
                                    g_srv_sock_elmt[i].clts[j].cport = clientaddr.sin_port;
                                    g_srv_sock_elmt[i].init(j);
                                    g_srv_sock_elmt[i].clt_num++;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            close(fd);
                        }
                    }
                }
            }
        }
        else if (ret < 0)
        {
            usleep(500);
        }

        for (i = 0; i < g_srv_sock_elmt_num; i++)
        {
            if (g_srv_sock_elmt[i].state == SOCK_FD_STATE_ACCEPT)
            {
                for (j = 0; j < SOCK_MAX_CLT_NUM; j++)
                {
                    if (g_srv_sock_elmt[i].clts[j].fd)
                    {
                        ret = g_srv_sock_elmt[i].send(j);
                        if (ret)
                        {
                            close(g_srv_sock_elmt[i].clts[j].fd);
                            g_srv_sock_elmt[i].clts[j].fd = 0;
                            g_srv_sock_elmt[i].clt_num--;
                        }
                    }
                }
            }
        }

    }

    for (i = 0; i < g_srv_sock_elmt_num; i++)
    {
        if (g_srv_sock_elmt[i].state != SOCK_FD_STATE_CLOSE)
        {
            for (j = 0; j < SOCK_MAX_CLT_NUM; j++)
            {
                if (g_srv_sock_elmt[i].clts[j].fd)
                {
                    close(g_srv_sock_elmt[i].clts[j].fd);
                    g_srv_sock_elmt[i].clts[j].fd = 0;
                }
            }
            close(g_srv_sock_elmt[i].fd);
            g_srv_sock_elmt[i].clt_num = 0;
            g_srv_sock_elmt[i].state = SOCK_FD_STATE_CLOSE;
        }
    }

    return NULL;
}

