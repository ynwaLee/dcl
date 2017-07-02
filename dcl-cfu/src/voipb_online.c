#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE   600    /* See feature_test_macros(7) */
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <alloca.h>

#include "asn.h"
#include "dcl_online.h"
#include "svm_online.h"
#include "mobile_online.h"
#include "voipb_online.h"
#include "conf_parser.h"
#include "mobonline_array.h"
#include "../../common/applog.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"


#define CLIENT_NUM      1024

sock_buf_node_t voipb_node[CLIENT_NUM];


void *thread_voipb_online(void *arg)
{
    int listenfd;
    int connfd;
    int nfds;
    fd_set readfds;
    struct timeval tv;
    uint32_t conn_times;
    int client_fd[CLIENT_NUM];
    int client_counter;
    char *recv_buf;
    unsigned int recv_len;
    unsigned int remain_len;
    int ret;
    int i;
    int data_flag;


    listenfd = -1;
    conn_times = 0;
    memset(client_fd, 0, sizeof(client_fd));
    client_counter = 0;
    for (i = 0; i < CLIENT_NUM; i++) {
        voipb_node[i].recv_len = 0;
        voipb_node[i].remain_len = MAX_BUF_SIZE;
        memset(voipb_node[i].recv_buf, 0, MAX_BUF_SIZE);
    }
    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }
        if (listenfd == -1) {
            listenfd = create_tcp_server(cfu_conf.online_voip_b_ip, cfu_conf.online_voip_b_port);
            if (listenfd == -1) {
                if (++conn_times >= 60) {
                    conn_times = 0;
                    applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "create tcp server failed");
                }
                sleep(1);
                continue;
            }
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);
        nfds = listenfd + 1;
        for (i = 0; i < CLIENT_NUM; i++) {
            if (client_fd[i] > 0) {
                FD_SET(client_fd[i], &readfds);
                if (client_fd[i] + 1 > nfds) {
                    nfds = client_fd[i] + 1;
                }
            }
        }
        ret = select(nfds, &readfds, NULL, NULL, &tv);
        if (ret <= 0) {
            continue;
        }

        if (FD_ISSET(listenfd, &readfds)) {
            applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                    "voip-b online connect to dcl");
            if((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
                applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, 
                        "voip-b server accept socket error: %s(errno: %d)",strerror(errno),errno);
                continue;
            }
            for (i = 0; i < CLIENT_NUM; i++) {
                if (client_fd[i] <= 0) {
                    client_fd[i] = connfd;
                    client_counter += 1;
                    applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                            "voip-b server add client %d to postion %d", client_fd[i], i);
                    break;
                }
            }
            if (i == CLIENT_NUM) {
                applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "voip-b server too many clients");
                close(connfd);
            }
        }

        for (i = 0; i < CLIENT_NUM; i++) {
            if (client_fd[i] > 0 && FD_ISSET(client_fd[i], &readfds)) {
                recv_buf = voipb_node[i].recv_buf;
                recv_len = voipb_node[i].recv_len;
                remain_len = voipb_node[i].remain_len;
                ret = read(client_fd[i], recv_buf + recv_len, remain_len - 1);
                if (ret <= 0) {
                    close(client_fd[i]);
                    applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, 
                            "voip-b read client %d return %d, close it", client_fd[i], ret);
                    client_fd[i] = -1;
                    recv_len = 0;
                    remain_len = MAX_BUF_SIZE;
                    continue;
                }
                applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                        "voip-b read client %d return %d bytes", client_fd[i], ret);
                recv_len += ret;
                remain_len -= ret;
                recv_buf[recv_len] = '\0';
                voipb_node[i].recv_len = recv_len;
                voipb_node[i].remain_len = remain_len;

#if 1
                /********************************************************/
                while (voipb_node[i].recv_len > 0) {
                    data_flag = check_data_complete((unsigned char *)recv_buf, voipb_node[i].recv_len);
                    applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                            "voip-b online check_data_complete return %d", data_flag);
                    if (data_flag > 0) {
                        int os = 0;
                        int len = data_flag;
                        mobile_online_t online = {0};
                        while(len > 0) {
                            unsigned int layer[32];
                            int asnlen = deasn((unsigned char *)recv_buf + os, len, &online, layer, 1, 32, asncallback);
                            os += asnlen;
                            len -= asnlen;
                        }
                        online.systype = SYSTYPE_VOIPB;
                        if (!strcmp(online.signal_action, "release")) {
                            mobonline_array_remove(&g_mobonline_array, online.systype, online.mntid, online.channel);
                            applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "voip-b Release: mntid = %d, channel = %d.", 
                                    online.mntid,
                                    online.channel);
                        } else if(!strcmp(online.signal_action, "call-establishment")) {
                            mobonline_array_add(&online, &g_mobonline_array);
                            applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "voip-b Call-Establishment: mntid = %d, channel = %d.", 
                                    online.mntid,
                                    online.channel);
                        }
                        if (voipb_node[i].recv_len > data_flag) {
                            memmove(voipb_node[i].recv_buf, voipb_node[i].recv_buf + data_flag, voipb_node[i].recv_len - data_flag);
                            voipb_node[i].recv_len = voipb_node[i].recv_len - data_flag;
                            voipb_node[i].remain_len = MAX_BUF_SIZE - voipb_node[i].recv_len;
                        } else {
                            voipb_node[i].recv_len = 0;
                            voipb_node[i].remain_len = MAX_BUF_SIZE;
                            memset(voipb_node[i].recv_buf, 0, MAX_BUF_SIZE);
                        }
                    } else if (data_flag == -1) {
                        voipb_node[i].recv_len = 0;
                        voipb_node[i].remain_len = MAX_BUF_SIZE;
                        memset(voipb_node[i].recv_buf, 0, MAX_BUF_SIZE);
                    } else {
                        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                                "voip-b online %d bytes data is not complete", voipb_node[i].recv_len);
                        break;
                    }
                }
                /********************************************************/
#else

                data_flag = check_data_complete((unsigned char *)recv_buf, recv_len);
                applog(APP_LOG_LEVEL_INFO, APP_VPU_LOG_MASK_BASE, 
                        "check_data_complete return %d", data_flag);
                if (data_flag > 0) {
                    int os = 0;
                    //int len = recv_len;
                    int len = data_flag;
                    mobile_online_t online = {0};
                    while(len > 0) {
                        unsigned int layer[32];
                        int asnlen = deasn((unsigned char *)recv_buf + os, len, &online, layer, 1, 32, asncallback);
                        os += asnlen;
                        len -= asnlen;
                    }
                    online.systype = SYSTYPE_VOIPB;
                    if (!strcmp(online.signal_action, "release")) {
                        mobonline_array_remove(&g_mobonline_array, online.systype, online.mntid, online.channel);
                        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "voip-b Release: mntid = %d, channel = %d.", 
                                online.mntid,
                                online.channel);
                    }
                    else if(!strcmp(online.signal_action, "call-establishment")) {
                        mobonline_array_add(&online, &g_mobonline_array);
                        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "voip-b Call-Establishment: mntid = %d, channel = %d.", 
                                online.mntid,
                                online.channel);
                    }
                    if (recv_len > data_flag) {
                        memmove(voipb_node[i].recv_buf, voipb_node[i].recv_buf + data_flag, recv_len - data_flag);
                        voipb_node[i].recv_len = recv_len - data_flag;
                        voipb_node[i].remain_len = MAX_BUF_SIZE - voipb_node[i].recv_len;
                    } else {
                        voipb_node[i].recv_len = 0;
                        voipb_node[i].remain_len = MAX_BUF_SIZE;
                        memset(voipb_node[i].recv_buf, 0, MAX_BUF_SIZE);
                    }
                } else if (data_flag == -1) {
                    voipb_node[i].recv_len = 0;
                    voipb_node[i].remain_len = MAX_BUF_SIZE;
                    memset(voipb_node[i].recv_buf, 0, MAX_BUF_SIZE);
                }
#endif
            }
        }
    }

    pthread_exit(NULL);


    return 0;
}
