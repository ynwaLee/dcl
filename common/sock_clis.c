
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "file_func.h"
#include "sock_clis.h"
#include "thread_flag.h"
#include "sock_cli.h"

STRUCT_SERVER_ELMT g_server_elmt[8];
unsigned char g_server_elmt_num = 0;
unsigned char g_server_reconnect = THREAD_OPR_FLAG_IDLE;


void *sock_server_proc(__attribute__((unused))void *arg)
{
    int  ret;
     int i;

    while (1)
    {
        if (svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))
            return NULL;

        if (g_server_reconnect == THREAD_OPR_FLAG_REQ)
        {
            for(i = 0; i < g_server_elmt_num; i++)
            {
                if (g_server_elmt[i].flag == SOCK_SRV_FLAG_RECONN)
                {
                    close(g_server_elmt[i].fd);
                    g_server_elmt[i].state = SOCK_FD_STATE_CLOSE;
                    g_server_elmt[i].flag = 0;
                }
            }
        }

        for(i = 0; i < g_server_elmt_num; i++)
        {
            if (g_server_elmt[i].state == SOCK_FD_STATE_CLOSE)
            {
                if (g_server_elmt[i].counter == 0)
                {
                    if ((g_server_elmt[i].fd = connect2srv(g_server_elmt[i].ip, g_server_elmt[i].port)) == -1)
                    {
                        g_server_elmt[i].counter = g_server_elmt[i].retry_interval;
                        usleep(1000);
                    }
                    else
                    {
                        g_server_elmt[i].state = SOCK_FD_STATE_INIT;
                        g_server_elmt[i].init(g_server_elmt[i].fd);
                        g_server_elmt[i].state = SOCK_FD_STATE_CONNECT;
                    }
                }
                else
                {
                    g_server_elmt[i].counter--;
                    usleep(200);
                }
            }
        }
        clear_file_set();
        for (i = 0; i < g_server_elmt_num; i++)
        {
            if (g_server_elmt[i].state == SOCK_FD_STATE_CONNECT)
            {
                check_file_add_fd(g_server_elmt[i].fd);
            }
        }
        ret = check_files_readable();
        if (ret > 0)
        {
            for (i = 0; i < g_server_elmt_num; i++)
            {
                if ((g_server_elmt[i].state == SOCK_FD_STATE_CONNECT)&&(file_readable(g_server_elmt[i].fd)))
                {
                    ret = g_server_elmt[i].recv(g_server_elmt[i].fd);
                    if (ret)
                    {
                        g_server_elmt[i].state = SOCK_FD_STATE_CLOSE;
                        close(g_server_elmt[i].fd);
                    }
                }
            }
        }
        else if (ret < 0)
        {
            usleep(500);
        }

        for (i = 0; i < g_server_elmt_num; i++)
        {
            if (g_server_elmt[i].state == SOCK_FD_STATE_CONNECT)
            {
                ret = g_server_elmt[i].send(g_server_elmt[i].fd);
                if (ret)
                {
                    g_server_elmt[i].state = SOCK_FD_STATE_CLOSE;
                    close(g_server_elmt[i].fd);
                }
            }
        }
    }

    for (i = 0; i < g_server_elmt_num; i++)
    {
        if (g_server_elmt[i].state != SOCK_FD_STATE_CLOSE)
            close(g_server_elmt[i].fd);
    }
    
    return NULL;
}







void *sock_clients_proc(__attribute__((unused))void *arg)
{
    int  ret;
     int i;

    while (1)
    {
        if (svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))
            return NULL;

        if (g_server_reconnect == THREAD_OPR_FLAG_REQ)
        {
            for(i = 0; i < g_server_elmt_num; i++)
            {
                if (g_server_elmt[i].flag == SOCK_SRV_FLAG_RECONN)
                {
                    close(g_server_elmt[i].fd);
                    g_server_elmt[i].state = SOCK_FD_STATE_CLOSE;
                    g_server_elmt[i].flag = 0;
                }
            }
        }

        for(i = 0; i < g_server_elmt_num; i++)
        {
            if (g_server_elmt[i].state == SOCK_FD_STATE_CLOSE)
            {
                if (g_server_elmt[i].counter == 0)
                {
                    if ((g_server_elmt[i].fd = connect2srv(g_server_elmt[i].ip, g_server_elmt[i].port)) == -1)
                    {
                        g_server_elmt[i].counter = g_server_elmt[i].retry_interval;
                        usleep(1000);
                    }
                    else
                    {
                        g_server_elmt[i].state = SOCK_FD_STATE_INIT;
                        g_server_elmt[i].init(g_server_elmt[i].fd);
                        g_server_elmt[i].state = SOCK_FD_STATE_CONNECT;
                    }
                }
                else
                {
                    g_server_elmt[i].counter--;
                    usleep(200);
                }
            }
        }
        clear_file_set();
        for (i = 0; i < g_server_elmt_num; i++)
        {
            if (g_server_elmt[i].state == SOCK_FD_STATE_CONNECT)
            {
                check_file_add_fd(g_server_elmt[i].fd);
            }
        }
        ret = check_files_readable();
        if (ret > 0)
        {
            for (i = 0; i < g_server_elmt_num; i++)
            {
                if ((g_server_elmt[i].state == SOCK_FD_STATE_CONNECT)&&(file_readable(g_server_elmt[i].fd)))
                {
                    if (g_server_elmt[i].remain_len == 0)
                    {
                        ret = g_server_elmt[i].recv(g_server_elmt[i].fd);
                        if (ret)
                        {
                            g_server_elmt[i].state = SOCK_FD_STATE_CLOSE;
                            close(g_server_elmt[i].fd);
                        }
                    }
                    else
                    {
                        ret = recv(g_server_elmt[i].fd,
                            &g_server_elmt[i].recv_data[g_server_elmt[i].recv_len],
                            g_server_elmt[i].remain_len, 0);
                        if (ret <= 0)
                        {
                            g_server_elmt[i].state = SOCK_FD_STATE_CLOSE;
                            close(g_server_elmt[i].fd);
                        }
                        else
                        {
                            g_server_elmt[i].remain_len -= ret;
                            g_server_elmt[i].recv_len += ret;
                            if (g_server_elmt[i].remain_len == 0)
                            {
                                ret = g_server_elmt[i].recv(g_server_elmt[i].fd);
                                if (ret)
                                {
                                    g_server_elmt[i].state = SOCK_FD_STATE_CLOSE;
                                    close(g_server_elmt[i].fd);
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (ret < 0)
        {
            usleep(500);
        }

        for (i = 0; i < g_server_elmt_num; i++)
        {
            if (g_server_elmt[i].state == SOCK_FD_STATE_CONNECT)
            {
                ret = g_server_elmt[i].send(g_server_elmt[i].fd);
                if (ret)
                {
                    g_server_elmt[i].state = SOCK_FD_STATE_CLOSE;
                    close(g_server_elmt[i].fd);
                }
            }
        }
    }

    for (i = 0; i < g_server_elmt_num; i++)
    {
        if (g_server_elmt[i].state != SOCK_FD_STATE_CLOSE)
            close(g_server_elmt[i].fd);
    }

    return NULL;
}

