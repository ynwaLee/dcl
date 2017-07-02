#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "cfu.h"
#include "svm_online.h"
#include "conf_parser.h"
#include "json/json.h"
#include "../../common/applog.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"

#define DCL_BUFSIZE         102400000

#define SVM_ONLINE_REQUEST  "{\"head\":{\"cmd\": 98}}"

svm_online_t svm_online;

int connect2server(char *ip, unsigned int port)
{
    int sockfd;
    struct sockaddr_in servaddr;
    struct timeval timeo = {3, 0};
    socklen_t len = sizeof(timeo);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "create socket fail");
        return -1;
    }

    //set the timeout period
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if(inet_pton(AF_INET, ip, &servaddr.sin_addr) != 1){  
        applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "inet_pton error for %s", ip);  
        close(sockfd);
        return -1; 
    }

    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
        if (errno == EINPROGRESS) {
            applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "connect to server timeout");
        }
        applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "connect to server error");
        close(sockfd);
        return -1;
    }
    applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "connect to server(ip: %s, port: %d) success", 
            ip, port);

    return sockfd;
}


int get_online_node(svm_online_node_t *node, json_object *obj)
{
    json_object *sub_obj;
    const char *str;

    if (node == NULL || obj == NULL) {
        return -1;
    }
    
    memset(node, 0, sizeof(svm_online_node_t));

    sub_obj = json_object_object_get(obj, "callid");
    if (sub_obj == NULL) {
        return -1;
    }
    str = json_object_get_string(sub_obj);
    node->callid = strtol(str, NULL, 10);

    sub_obj = json_object_object_get(obj, "ruleid");
    if (sub_obj == NULL) {
        return -1;
    }
    node->ruleid = json_object_get_int(sub_obj);

    sub_obj = json_object_object_get(obj, "dialtime");
    if (sub_obj == NULL) {
        return -1;
    }
    str = json_object_get_string(sub_obj);
    node->dialtime = strtol(str, NULL, 10);

    sub_obj = json_object_object_get(obj, "callfrom");
    if (sub_obj == NULL) {
        return -1;
    }
    str = json_object_get_string(sub_obj);
    strncpy(node->fromphone, str, 31);

    sub_obj = json_object_object_get(obj, "callto");
    if (sub_obj == NULL) {
        return -1;
    }
    str = json_object_get_string(sub_obj);
    strncpy(node->tophone, str, 31);

    sub_obj = json_object_object_get(obj, "userid");
    if (sub_obj == NULL) {
        return -1;
    }
    node->userid = json_object_get_int(sub_obj);

    sub_obj = json_object_object_get(obj, "voiceid");
    if (sub_obj == NULL) {
        return -1;
    }
    node->voiceid = json_object_get_int(sub_obj);

    sub_obj = json_object_object_get(obj, "percent");
    if (sub_obj == NULL) {
        return -1;
    }
    node->percent = json_object_get_int(sub_obj);

    //applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "callid: %lu, ruleid: %u", node->callid, node->ruleid);

    return 0;
}



char recv_data[DCL_BUFSIZE];


void *thread_svm_online(void *arg)
{
    int sockfd;
    int ret;
    uint32_t conn_times;
    uint32_t remain_len;
    uint32_t recv_len;
    json_object *obj;
    json_object *sub_obj;
    json_object *array_obj;
    svm_online_node_t node;
    int operation;
    int array_num;
    int i;

    memset(&svm_online, 0, sizeof(svm_online_t));
    pthread_mutex_init(&svm_online.mutex, NULL);
    
    sockfd = -1;
    conn_times = 0;
    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }
        if (sockfd == -1) {
            if ((sockfd = connect2server(cfu_conf.online_svm_ip, cfu_conf.online_svm_port)) == -1) {
                if (++conn_times >= 60) {
                    conn_times = 0;
                    applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "connect to svm cdr failed");
                }
                sleep(1);
                continue;
            }
            conn_times = 0;
            remain_len = DCL_BUFSIZE;
            recv_len = 0;
        }

        write(sockfd, SVM_ONLINE_REQUEST, strlen(SVM_ONLINE_REQUEST));

        remain_len = DCL_BUFSIZE;
        recv_len = 0;
        FILE *fp_out = fopen("/dev/shm/svm_online", "w+");
        while ((ret = read(sockfd, recv_data + recv_len, remain_len)) > 0) {
            fwrite(recv_data + recv_len, 1, ret, fp_out);
            recv_len += ret;
            remain_len -= ret;
            if (recv_len + 1 >= DCL_BUFSIZE) {
                //applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "receive svm online string len %d", recv_len);
                break;
            }
        }
        fclose(fp_out);
        recv_data[recv_len] = '\0';
        close(sockfd);
        sockfd = -1;
        //applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "receive svm online string %s", recv_data);

        obj = json_tokener_parse(recv_data);
        if (is_error(obj)) {
            applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "json_tokener_parse error, json string: %s", recv_data);
            continue;
        }
        if (json_object_get_type(obj) != json_type_object) {
            applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "object type error");
            json_object_put(obj);
            continue;
        }

        sub_obj = json_object_object_get(obj, "operation");
        if (sub_obj == NULL) {
            json_object_put(obj);
            continue;
        }
        operation = json_object_get_int(sub_obj);
        if (operation != 1) {
            json_object_put(obj);
            continue;
        }
        
        sub_obj = json_object_object_get(obj, "msg");
        if (sub_obj == NULL) {
            json_object_put(obj);
            continue;
        }
        array_num = json_object_array_length(sub_obj);
        applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "json array num: %d", array_num);

        pthread_mutex_lock(&svm_online.mutex);
        svm_online.counter = 0;
        //do someting
        for (i = 0; i < array_num; i++) {
            memset(&node, 0, sizeof(svm_online_node_t));
            array_obj = json_object_array_get_idx(sub_obj, i);
            if (array_obj == NULL) {
                continue;
            }
            if (get_online_node(&node, array_obj) == 0) {
                memcpy(&svm_online.node[svm_online.counter], &node, sizeof(svm_online_node_t));
                svm_online.counter += 1;
                if (svm_online.counter >= SVM_ONLINE_NUM) {
                    applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "%d online call, too many for cfu", svm_online.counter);
                    break;
                }
            }
        }
        pthread_mutex_unlock(&svm_online.mutex);
        applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "%d online call", svm_online.counter);

        json_object_put(obj);
        sleep(5);
    }

    pthread_exit(NULL);
}


