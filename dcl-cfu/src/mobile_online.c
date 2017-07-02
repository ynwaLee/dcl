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
#include "conf_parser.h"
#include "mobonline_array.h"
#include "../../common/applog.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"


#define CLIENT_NUM      1024

sock_buf_node_t node[CLIENT_NUM];



int check_data_complete(unsigned char *data, uint32_t len)
{
    uint8_t flag;
    uint32_t data_len;

    if (data == NULL || len <= 0 || data[0] != 0x30) {
        return -1;
    }

    flag = data[1];
    if ((flag & 0x80) == 0) {
        data_len = flag;
        if (len >= data_len + 2) {
            return (data_len + 2);
        } else {
            return 0;
        }
    } else {
        flag = flag & 0x7f;
        if (flag > 4) {
            return -1;
        }

        int i;
        data_len = 0;
        for (i = 0; i < flag; i++) {
            data_len = (data_len << 8) | data[2 + i];
        }

        if (len >= data_len + 2 + flag) {
            return (data_len + 2 + flag);
        } else {
            return 0;
        }
    }

    return 0;
}






void asncallback(unsigned int tag, int constructed, unsigned char * content, int contentlen, unsigned int *layer, int curlayer, void * para)
{
    int i;
    mobile_online_t *online = (mobile_online_t *)para;
    online->recv_time = time(0);

    if (constructed != 0) {
        return ;
    }

    //1. MNTID
    if (curlayer == 3 && tag == 0x81) {
        unsigned char cmplayer[] = {0x30, 0xA1, 0x81};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen < 20) {
            char *buf = alloca(64);
            memset(buf, 0, 64);
            memcpy(buf, content, contentlen);
            buf[contentlen] = 0;
            online->mntid = atoi(buf);
            online->mntid -= 200;
            applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "MNTID: %d", online->mntid);
        }
    }

    //Operator
    if (curlayer == 5 && tag == 0x80) {
        unsigned char cmplayer[] = {0x30, 0xA1, 0xA3, 0xA0, 0x80};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res) {
            char *buf = alloca(contentlen + 1);
            memset(buf, 0, contentlen + 1);
            memcpy(buf, content, contentlen);
            buf[contentlen] = 0;
            snprintf(online->operator, 64, "%s", buf);
            applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "Operator: %s", online->operator);
        }
    }

    //2. channel
    if (curlayer == 4 && tag == 0x81) {
        unsigned char cmplayer[] = {0x30, 0xA1, 0xA3, 0x81};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen <= 8) {
            unsigned char *buf = alloca(8);
            memset(buf, 0, 8);
            memcpy(buf, content, contentlen);
            uint32_t callcode = *(unsigned int *)buf;
            online->channel = contentlen == 0 ? -1 : callcode;
            applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "callcode: %u", online->channel);
        }
    }


    //time
    if (curlayer == 3 && tag == 0x85) {
        unsigned char cmplayer[] = {0x30, 0xA1, 0x85};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res) {
            char *buf = alloca(contentlen + 1);
            memset(buf, 0, contentlen + 1);
            memcpy(buf, content, contentlen);
            buf[contentlen] = 0;

            struct tm tm = {0};
            //%Y-%m-%d %H:%M:%S
            strptime(buf, "%Y%m%d%H%M%S", &tm);
            online->time = mktime(&tm);
            applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "time: %s, timestamp: %lu", buf, online->time);
        }
    }

    //Call direction
    if (curlayer == 9 && tag == 0x84) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA1, 0x84};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen <= 8) {
            unsigned char *buf = alloca(8);
            memset(buf, 0, 8);
            memcpy(buf, content, contentlen);
            uint32_t calldir = *(unsigned int *)buf;
            online->calldir = calldir;
            applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "Call direction: %u", online->calldir);
            if (online->calldir == 0x02) {
                online->aaction = 1;
                applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "action: called");
            } else {
                online->aaction = 0;
                applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "action: calling");
            }
        }
    }

    //CGI
    if (curlayer == 10 && tag == 0x82) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA1, 0xA8, 0x82};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen <= 8) {
            unsigned char *buf = alloca(8);
            memset(buf, 0, 8);
            memcpy(buf, content, contentlen);

            int mcc;
            int mnc;
            int lac;
            int cellid;
            unsigned char tmp;
            tmp = content[0];
            mcc = tmp & 0x0f;
            mcc = (mcc * 10) + ((tmp >> 4) & 0x0f);
            tmp = content[1];
            mcc = (mcc * 10) + (tmp & 0x0f);

            tmp = content[2];
            mnc = tmp & 0x0f;
            mnc = (mnc * 10) + ((tmp>>4) & 0x0f);

            tmp = content[3];
            lac = tmp;
            tmp = content[4];
            lac = (lac << 8) + tmp;

            tmp = content[5];
            cellid = tmp;
            tmp = content[6];
            cellid = (cellid << 8) + tmp;

            online->mcc = mcc;
            online->mnc = mnc;
            online->cellid = cellid;
            applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "MCC: %03d, MNC: %02d, LAC: %d, CELLID: %d", 
                    online->mcc, 
                    online->mnc, 
                    online->lac, 
                    online->cellid);
        }
    }

#if 0
    //A action
    if (curlayer == 11 && tag == 0x80) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA4, 0xA9, 0x30, 0x80};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen <= 8) {
            unsigned char *buf = alloca(8);
            memset(buf, 0, 8);
            memcpy(buf, content, contentlen);
            uint8_t calldir = buf[0];
            if (calldir == 0) {
                applog(APP_LOG_LEVEL_INFO, APP_VPU_LOG_MASK_BASE, "A action: calling");
            } else {
                applog(APP_LOG_LEVEL_INFO, APP_VPU_LOG_MASK_BASE, "A action: called");
            }
            online->aaction = calldir;
        }
    }
#endif

#if 0
    //A IMEI
    if (curlayer == 12 && tag == 0x81) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA4, 0xA9, 0x30, 0xA1, 0x81};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen <= 8) {
            unsigned char *buf = alloca(16);
            memset(buf, 0, 16);
            int i, j;
            uint8_t tmp;
            for (i = 0, j = 0; i < contentlen; i++) {
                tmp = content[i] & 0x0f;
                buf[j] = tmp + 0x30;
                if (j == 14) {
                    break;
                }
                tmp = (content[i] >> 4) & 0x0f;
                buf[j + 1] = tmp + 0x30;
                j += 2;
            }
            applog(APP_LOG_LEVEL_INFO, APP_VPU_LOG_MASK_BASE, "A IMEI: %s", buf);
            snprintf(online->aimei, 64, "%s", buf);
        }
#endif

#if 0
        if (call_dir == 1 && num_type == 0) {
            //A
        } else if (call_dir == 1 && num_type == 1) {
            //B
        } else if (call_dir == 2 && num_type == 0) {
            //B
        } else if (call_dir == 2 && num_type == 1) {
            //A
        }
    }
#endif



    //A IMSI
    if (curlayer == 12 && tag == 0x83) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA1, 0xA9, 0x30, 0xA1, 0x83};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen <= 8) {
            unsigned char *buf = alloca(16);
            memset(buf, 0, 16);
            int i, j;
            uint8_t tmp;
            for (i = 0, j = 0; i < contentlen; i++) {
                tmp = content[i] & 0x0f;
                if (tmp > 9) {
                    break;
                }
                buf[j] = tmp + 0x30;

                tmp = (content[i] >> 4) & 0x0f;
                if (tmp > 9) {
                    break;
                }
                buf[j + 1] = tmp + 0x30;

                j += 2;
                if (j == 14) {
                    break;
                }
            }
            snprintf(online->aimsi, 64, "%s", buf);
            applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "A IMSI: %s", online->aimsi);
        }
    }


    //called number
    if (curlayer == 13 && tag == 0x81) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA1, 0xA9, 0x30, 0xA1, 0xA5, 0x81};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen <= 16) {
            unsigned char *buf = alloca(16);
            memset(buf, 0, 16);
            int i, j;
            uint8_t tmp;
            for (i = 2, j = 0; i < contentlen; i++) {
                tmp = content[i] & 0x0f;
                if (tmp > 9) {
                    break;
                }
                buf[j] = tmp + 0x30;

                tmp = (content[i] >> 4) & 0x0f;
                if (tmp > 9) {
                    break;
                }
                buf[j + 1] = tmp + 0x30;

                j += 2;
                if (j == 14) {
                    break;
                }
            }
            if (online->calldir == 0x02) {
                snprintf(online->amsisdn, 64, "%s", buf);
                applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "called number: %s", online->amsisdn);
            } else {
                snprintf(online->bphonenum, 64, "%s", buf);
                applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "called number: %s", online->bphonenum);
            }
        }
    }


#if 0
    //B action
    if (curlayer == 11 && tag == 0x80) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA4, 0xA9, 0x30, 0x80};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen <= 8) {
            unsigned char *buf = alloca(8);
            memset(buf, 0, 8);
            memcpy(buf, content, contentlen);
            uint8_t calldir = buf[0];
            if (calldir == 0) {
                applog(APP_LOG_LEVEL_INFO, APP_VPU_LOG_MASK_BASE, "B action: calling");
            } else {
                applog(APP_LOG_LEVEL_INFO, APP_VPU_LOG_MASK_BASE, "B action: called");
            }
            online->baction = calldir;
        }
    }
#endif

    //calling number
    if (curlayer == 13 && tag == 0x81) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA1, 0xA9, 0x30, 0xA1, 0xA4, 0x81};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen <= 16) {
            unsigned char *buf = alloca(32);
            memset(buf, 0, 32);
            int i, j;
            uint8_t tmp;
            for (i = 2, j = 0; i < contentlen; i++) {
                tmp = content[i] & 0x0f;
                if (j > 30 || tmp > 9) {
                    break;
                }
                buf[j] = tmp + 0x30;

                tmp = (content[i] >> 4) & 0x0f;
                if (j > 30 || tmp > 9) {
                    break;
                }
                buf[j + 1] = tmp + 0x30;

                j += 2;
            }
            if (online->calldir == 0x02) {
                snprintf(online->bphonenum, 64, "%s", buf);
                applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "calling number: %s", online->bphonenum);
            } else {
                snprintf(online->amsisdn, 64, "%s", buf);
                applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "calling number: %s", online->amsisdn);
            }
        }
    }

#if 0
    //SMS content
    if (curlayer == 11 && tag == 0x84) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA4, 0xAE, 0xA3, 0x84};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res) {
            unsigned char *buf = alloca(contentlen + 1);
            memset(buf, 0, contentlen + 1);
            memcpy(buf, content, contentlen);
            applog(APP_LOG_LEVEL_INFO, APP_VPU_LOG_MASK_BASE, "SMS: %s", buf);
            snprintf(online->smscontent, 1024, "%s", buf);
        }
    }
#endif

    //signal action
    if (curlayer == 9 && tag == 0x21) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA0, 0x30, 0xA2, 0xA4, 0xA3, 0xA1, 0x21};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (i == 7) {
                continue;
            }
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res) {
            uint8_t action = content[0];
            char *ac_msg[] = {"call-establishment", "answer", "supplementary-Service", "handover", "release", "sMS", "location-update", "subscriber-Controlled-Input", "periodic-update", "power-on", "power-off", "authenticate", "paging"};

            snprintf(online->signal_action, 64, "%s", ac_msg[action - 1]);
            applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "event: %d  %s", action, online->signal_action);
        }
    }

    return ;
}
















void *thread_mobile_online(void *arg)
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
        node[i].recv_len = 0;
        node[i].remain_len = MAX_BUF_SIZE;
        memset(node[i].recv_buf, 0, MAX_BUF_SIZE);
    }
    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }
        if (listenfd == -1) {
            listenfd = create_tcp_server(cfu_conf.online_mobile_ip, cfu_conf.online_mobile_port);
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
                    "mobile online connect to dcl");
            if((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
                applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, 
                        "mobile online server accept socket error: %s(errno: %d)",strerror(errno),errno);
                continue;
            }
            for (i = 0; i < CLIENT_NUM; i++) {
                if (client_fd[i] <= 0) {
                    client_fd[i] = connfd;
                    client_counter += 1;
                    applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                            "mobile online server add client %d to postion %d", client_fd[i], i);
                    break;
                }
            }
            if (i == CLIENT_NUM) {
                applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "mobile online server has too many clients");
                close(connfd);
            }
        }

        for (i = 0; i < CLIENT_NUM; i++) {
            if (client_fd[i] > 0 && FD_ISSET(client_fd[i], &readfds)) {
                recv_buf = node[i].recv_buf;
                recv_len = node[i].recv_len;
                remain_len = node[i].remain_len;
                ret = read(client_fd[i], recv_buf + recv_len, remain_len - 1);
                if (ret <= 0) {
                    close(client_fd[i]);
                    applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                            "mobile online read client %d return %d, close it", client_fd[i], ret);
                    client_fd[i] = -1;
                    recv_len = 0;
                    remain_len = MAX_BUF_SIZE;
                    continue;
                }
                recv_len += ret;
                remain_len -= ret;
                recv_buf[recv_len] = '\0';
                node[i].recv_len = recv_len;
                node[i].remain_len = remain_len;

#if 1
                while (node[i].recv_len > 0) {
                    data_flag = check_data_complete((unsigned char *)recv_buf, node[i].recv_len);
                    applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                            "mobile online check_data_complete return %d", data_flag);
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
                        online.systype = SYSTYPE_MOBILE;
                        if (!strcmp(online.signal_action, "release")) {
                            mobonline_array_remove(&g_mobonline_array, online.systype, online.mntid, online.channel);
                            applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "moblie Release: mntid = %d, channel = %d.", 
                                    online.mntid,
                                    online.channel);
                        } else if(!strcmp(online.signal_action, "call-establishment")) {
                            mobonline_array_add(&online, &g_mobonline_array);
                            applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "mobile Call-Establishment: mntid = %d, channel = %d.", 
                                    online.mntid,
                                    online.channel);
                        }
                        if (node[i].recv_len > data_flag) {
                            memmove(node[i].recv_buf, node[i].recv_buf + data_flag, node[i].recv_len - data_flag);
                            node[i].recv_len = node[i].recv_len - data_flag;
                            node[i].remain_len = MAX_BUF_SIZE - node[i].recv_len;
                        } else {
                            node[i].recv_len = 0;
                            node[i].remain_len = MAX_BUF_SIZE;
                            memset(node[i].recv_buf, 0, MAX_BUF_SIZE);
                        }
                    } else if (data_flag == -1) {
                        node[i].recv_len = 0;
                        node[i].remain_len = MAX_BUF_SIZE;
                        memset(node[i].recv_buf, 0, MAX_BUF_SIZE);
                    } else {
                        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                                "mobile online %d bytes data is not complete", node[i].recv_len);
                        break;
                    }
                }
#else
                data_flag = check_data_complete((unsigned char *)recv_buf, recv_len);
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
                    online.systype = SYSTYPE_MOBILE;
                    if (!strcmp(online.signal_action, "release")) {
                        mobonline_array_remove(&g_mobonline_array, online.systype, online.mntid, online.channel);
                        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "moblie Release: mntid = %d, channel = %d.", 
                                online.mntid,
                                online.channel);
                    }
                    else if(!strcmp(online.signal_action, "call-establishment")) {
                        mobonline_array_add(&online, &g_mobonline_array);
                        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "mobile Call-Establishment: mntid = %d, channel = %d.", 
                                online.mntid,
                                online.channel);
                    }
                    if (recv_len > data_flag) {
                        memmove(node[i].recv_buf, node[i].recv_buf + data_flag, recv_len - data_flag);
                        node[i].recv_len = recv_len - data_flag;
                        node[i].remain_len = MAX_BUF_SIZE - node[i].recv_len;
                    } else {
                        node[i].recv_len = 0;
                        node[i].remain_len = MAX_BUF_SIZE;
                        memset(node[i].recv_buf, 0, MAX_BUF_SIZE);
                    }
                } else if (data_flag == -1) {
                    node[i].recv_len = 0;
                    node[i].remain_len = MAX_BUF_SIZE;
                    memset(node[i].recv_buf, 0, MAX_BUF_SIZE);
                }
#endif
            }
        }
    }

    pthread_exit(NULL);


    return 0;
}
