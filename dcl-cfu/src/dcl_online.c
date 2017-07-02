#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "dcl_online.h"
#include "svm_online.h"
#include "mobile_online.h"
#include "mobonline_array.h"
#include "conf_parser.h"
#include "libxml/parser.h"
#include "libxml/xmlmemory.h"
#include "libxml/tree.h"
#include "../../common/applog.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"



#define CLIENT_NUM      1024
#define ONLINE_SERVER_PORT      20001

#define ONLINE_XML_FILE         "/dev/shm/online_xml"



int create_tcp_server(char *ip, uint16_t port)
{
    int listenfd;
    struct sockaddr_in servaddr;
    int reuseaddr = 1;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "create_tcp_server ip %s port %u create socket fail", ip, port);
        return -1;
    }

    if (port == 0) {
        port = ONLINE_SERVER_PORT;
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    if (ip == NULL) {
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        servaddr.sin_addr.s_addr = inet_addr(ip);
    }
    servaddr.sin_port = htons(port);

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "create_tcp_server ip %s port %u bind socket fail", ip, port);
        close(listenfd);
        return -1;
    }

    if(listen(listenfd, 2048) == -1){
        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "create_tcp_server ip %s port %u listen socket fail", ip, port);
        close(listenfd);
        return -1;
    }

    return listenfd;
}



int get_object_clue(char *str, obj_clue_t *obj)
{
    xmlDocPtr pdoc;
    xmlNodePtr pnode_root;
    xmlAttrPtr pattr_var;
    xmlChar *pc_content;
    int object;
    int clue;

    if (str == NULL || obj == NULL) {
        return -1;
    }
    memset(obj, 0, sizeof(obj_clue_t));

    if((pdoc = xmlParseMemory(str, strlen(str))) == NULL ) {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "call xmlReadFile(%s) function failed", str);
        return -1;
    }

    if((pnode_root = xmlDocGetRootElement(pdoc)) == NULL) {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "call xmlDocGetRootElement(%s) functioan failed", str);
        xmlFreeDoc(pdoc);
        return -1;
    }

    if(xmlStrcmp(pnode_root->name, BAD_CAST"CALL_REQUEST") != 0) {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "xml no CALL_REQUEST");
        xmlFreeDoc(pdoc);
        return -1;
    }

    pnode_root = pnode_root->xmlChildrenNode;
    while (pnode_root != NULL) {
        pattr_var = pnode_root->properties;

        object = -1;
        clue = -1;
        while (pattr_var != NULL) {
            if (xmlStrcmp(pattr_var->name, (const xmlChar *)"OBJECT_ID") == 0) {
                pc_content = xmlGetProp(pnode_root, BAD_CAST"OBJECT_ID");
                object = strtol((const char *)pc_content, NULL, 10);
                xmlFree(pc_content);
            } else if (xmlStrcmp(pattr_var->name, (const xmlChar *)"CLUE_ID") == 0) {
                pc_content = xmlGetProp(pnode_root, BAD_CAST"CLUE_ID");
                clue = strtol((const char *)pc_content, NULL, 10);
                xmlFree(pc_content);
            }
            pattr_var = pattr_var->next;
        }
        applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "get object id %d, clue id %d from cbp", object, clue);
        if (object != -1 && clue != -1) {
            obj->node[obj->counter].obj = object;
            obj->node[obj->counter].clue = clue;
            obj->counter += 1;
        }

        pnode_root = pnode_root->next;
    }

    xmlFreeDoc(pdoc);

    return 0;
}


#define add_new_svm_node(pnode_child, svm_node, clue_node, ruleid) \
    do{\
        char xml_str[512] = {0};\
        xmlNewProp(pnode_child, BAD_CAST"SYS_TYPE", (xmlChar *)"1");\
        xmlNewProp(pnode_child, BAD_CAST"ONLINE", (xmlChar *)"1");\
        snprintf(xml_str, sizeof(xml_str) - 1, "%u", clue_node.obj);\
        xmlNewProp(pnode_child, BAD_CAST"OBJECT_ID", (xmlChar *)xml_str);\
        snprintf(xml_str, sizeof(xml_str) - 1, "%u", ruleid);\
        xmlNewProp(pnode_child, BAD_CAST"CLUE_ID", (xmlChar *)xml_str);\
        snprintf(xml_str, sizeof(xml_str) - 1, "%lu", svm_node.callid);\
        xmlNewProp(pnode_child, BAD_CAST"CALL_ID", (xmlChar *)xml_str);\
        xmlNewProp(pnode_child, BAD_CAST"CALLCODE", (xmlChar *)"0");\
        xmlNewProp(pnode_child, BAD_CAST"APHONE_NUM", (xmlChar *)svm_node.fromphone);\
        xmlNewProp(pnode_child, BAD_CAST"AIMSI", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"BPHONE_NUM", (xmlChar *)svm_node.tophone);\
        xmlNewProp(pnode_child, BAD_CAST"BIMSI", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"ACTION", (xmlChar *)"05");\
        snprintf(xml_str, sizeof(xml_str) - 1, "%lu", svm_node.dialtime);\
        xmlNewProp(pnode_child, BAD_CAST"TIME", (xmlChar *)xml_str);\
        if (svm_node.userid > 0) {\
            snprintf(xml_str, sizeof(xml_str) - 1, "%u", svm_node.userid);\
            xmlNewProp(pnode_child, BAD_CAST"CBID", (xmlChar *)xml_str);\
            snprintf(xml_str, sizeof(xml_str) - 1, "%u", svm_node.voiceid);\
            xmlNewProp(pnode_child, BAD_CAST"VID", (xmlChar *)xml_str);\
            snprintf(xml_str, sizeof(xml_str) - 1, "%u", svm_node.percent);\
            xmlNewProp(pnode_child, BAD_CAST"SCORE", (xmlChar *)xml_str);\
        } else {\
            xmlNewProp(pnode_child, BAD_CAST"CBID", (xmlChar *)"");\
            xmlNewProp(pnode_child, BAD_CAST"VID", (xmlChar *)"");\
            xmlNewProp(pnode_child, BAD_CAST"SCORE", (xmlChar *)"");\
        }\
        xmlNewProp(pnode_child, BAD_CAST"VOCFILE", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"CALLFLAG", (xmlChar *)"0");\
        xmlNewProp(pnode_child, BAD_CAST"C1", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"C2", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"I1", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"I2", (xmlChar *)"");\
    }while(0)

#define add_new_mobile_node(pnode_child, mobile_node, clue_node, ruleid) \
    do{\
        char xml_str[512] = {0};\
        xmlNewProp(pnode_child, BAD_CAST"CITY_CODE", (xmlChar *)(cfu_conf.src_city_code));\
        snprintf(xml_str, sizeof(xml_str) - 1, "%u", mobile_node.systype);\
        xmlNewProp(pnode_child, BAD_CAST"SYS_TYPE", (xmlChar *)xml_str);\
        xmlNewProp(pnode_child, BAD_CAST"ONLINE", (xmlChar *)"1");\
        snprintf(xml_str, sizeof(xml_str) - 1, "%u", clue_node.obj);\
        xmlNewProp(pnode_child, BAD_CAST"OBJECT_ID", (xmlChar *)xml_str);\
        snprintf(xml_str, sizeof(xml_str) - 1, "%u", ruleid);\
        xmlNewProp(pnode_child, BAD_CAST"CLUE_ID", (xmlChar *)xml_str);\
        uint64_t callid = (mobile_node.recv_time << 32) | (mobile_node.mntid << 16) | (mobile_node.channel & 0xffff);\
        snprintf(xml_str, sizeof(xml_str), "%lu", callid);\
        xmlNewProp(pnode_child, BAD_CAST"CALL_ID", (xmlChar *)xml_str);\
        snprintf(xml_str, sizeof(xml_str) - 1, "%u", mobile_node.channel);\
        xmlNewProp(pnode_child, BAD_CAST"CALLCODE", (xmlChar *)xml_str);\
        xmlNewProp(pnode_child, BAD_CAST"APHONE_NUM", (xmlChar *)mobile_node.amsisdn);\
        xmlNewProp(pnode_child, BAD_CAST"AIMSI", (xmlChar *)mobile_node.aimsi);\
        xmlNewProp(pnode_child, BAD_CAST"BPHONE_NUM", (xmlChar *)mobile_node.bphonenum);\
        xmlNewProp(pnode_child, BAD_CAST"BIMSI", (xmlChar *)"");\
        snprintf(xml_str, sizeof(xml_str), "%s", (mobile_node.aaction == 1) ? "05" : "06");\
        xmlNewProp(pnode_child, BAD_CAST"ACTION", (xmlChar *)xml_str);\
        snprintf(xml_str, sizeof(xml_str) - 1, "%lu", mobile_node.time);\
        xmlNewProp(pnode_child, BAD_CAST"TIME", (xmlChar *)xml_str);\
        xmlNewProp(pnode_child, BAD_CAST"CBID", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"VID", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"SCORE", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"VOCFILE", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"CALLFLAG", (xmlChar *)"0");\
        xmlNewProp(pnode_child, BAD_CAST"C1", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"C2", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"I1", (xmlChar *)"");\
        xmlNewProp(pnode_child, BAD_CAST"I2", (xmlChar *)"");\
    }while(0)



#define add_null_svm_node(pnode_child) \
    do{\
        xmlNewProp(pnode_child, BAD_CAST"ONLINE_NUM", (xmlChar *)"0");\
    }while(0)




int get_online_call(obj_clue_t *obj)
{
    xmlDocPtr pdoc;
    xmlNodePtr pnode_root;
    xmlNodePtr pnode_child;
    int online_num;
    int clue_num;
    int matched_num;
    int ruleid;
    int i, j;

    if((pdoc = xmlNewDoc(BAD_CAST"1.0")) == NULL) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "xmlNewDoc failed");
        return -1;
    }

    pnode_root = xmlNewNode(NULL, BAD_CAST"CALL_INFO");
    xmlDocSetRootElement(pdoc, pnode_root);

    pthread_mutex_lock(&svm_online.mutex);

    matched_num = 0;
    online_num = svm_online.counter;
    for (i = 0; i < online_num; i++) {
        ruleid = svm_online.node[i].ruleid;
        clue_num = obj->counter;
        for (j = 0; j < clue_num; j++) {
            if (ruleid == obj->node[j].clue || obj->node[j].clue == 0) {
                //applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_BASE, "get a online callid %lu match rule %d", svm_online.node[i].callid, ruleid);
                //this is what we want
                pnode_child = xmlNewNode(NULL, BAD_CAST"CALL");
                add_new_svm_node(pnode_child, svm_online.node[i], obj->node[j], ruleid);
                xmlAddChild(pnode_root, pnode_child);
                matched_num += 1;
                break;
            }
        }
    }

    pthread_mutex_unlock(&svm_online.mutex);

    /* mobile online */
    mobonline_array_t *array = &g_mobonline_array;

    pthread_mutex_lock(&array->mutex);

    mobile_online_t zero_online = {0}; 
    time_t curr_time = time(0);
    long max_session_time = 5*60*60;
    for (i = 0; i < MAX_MOBONLINE_NUM; i++) {
        if (bcmp(&array->node[i], &zero_online, sizeof(mobile_online_t)) != 0) {
            if (curr_time - array->node[i].recv_time < max_session_time) {
                ruleid = array->node[i].mntid;
                clue_num = obj->counter;
                for (j = 0; j < clue_num; j++) {
                    if (ruleid == obj->node[j].clue || obj->node[j].clue == 0) {
                        pnode_child = xmlNewNode(NULL, BAD_CAST"CALL");
                        add_new_mobile_node(pnode_child, array->node[i], obj->node[j], ruleid);
                        xmlAddChild(pnode_root, pnode_child);
                        matched_num += 1;
                        break;
                    }
                }
            } else {
                applog(LOG_INFO, APP_LOG_MASK_BASE, "Warning: Deleted a moible online cdr because of timeout. mntid: %d, channel: %d.", array->node[i].mntid, array->node[i].channel);
                bzero(&array->node[i], sizeof(mobile_online_t));
            }
        }
    }

    pthread_mutex_unlock(&array->mutex);

#if 0
    if (matched_num == 0) {
        pnode_child = xmlNewNode(NULL, BAD_CAST"CALL");
        add_null_svm_node(pnode_child);
        xmlAddChild(pnode_root, pnode_child);
    }
#endif
    //xmlSaveFile(ONLINE_XML_FILE, pdoc);
    xmlSaveFormatFileEnc(ONLINE_XML_FILE, pdoc, "UTF-8", 1);
    //applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "write online xml to %s", ONLINE_XML_FILE);
    xmlFreeDoc(pdoc);

    return 0;
}


int send_online_call(char *src, int sock)
{
    int fd_src;
    char buf[2048];
    int len;

    if (src == NULL || sock < 0) {
        return -1;
    }

    fd_src = open(src, O_RDONLY);
    if (fd_src == -1) {
        applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "open %s failed", src);
        return -1;
    }

    while ((len = read(fd_src, buf, 2048)) > 0) {
        write(sock, buf, len);
    }
    write(sock, "\n", 1);

    close(fd_src);

    return 0;
}




sock_buf_node_t node[CLIENT_NUM];

void *thread_dcl_online(void *arg)
{
    int listenfd;
    int connfd;
    int nfds;
    fd_set readfds;
    struct timeval tv;
    uint32_t conn_times;
    int client_fd[CLIENT_NUM];
    int client_counter;
    obj_clue_t obj_clue;
    char *recv_buf;
    unsigned int recv_len;
    unsigned int remain_len;
    int ret;
    int i;


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
            listenfd = create_tcp_server(cfu_conf.online_local_ip, cfu_conf.online_local_port);
            if (listenfd == -1) {
                if (++conn_times >= 60) {
                    conn_times = 0;
                    applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "create file server failed");
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
                    "das connect to dcl");
            if((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
                applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, 
                        "dcl online server accept socket error: %s(errno: %d)",strerror(errno),errno);
                continue;
            }
            for (i = 0; i < CLIENT_NUM; i++) {
                if (client_fd[i] <= 0) {
                    client_fd[i] = connfd;
                    client_counter += 1;
                    applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                            "dcl online server add client %d to postion %d", client_fd[i], i);
                    break;
                }
            }
            if (i == CLIENT_NUM) {
                applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "dcl online server has too many clients");
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
                            "read client %d return %d, close it", client_fd[i], ret);
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

                if (recv_len >= 5 && memcmp(recv_buf, "<?xml", 5) != 0) {
                    applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, 
                            "not xml string: %s", recv_buf);
                    node[i].recv_len = 0;
                    node[i].remain_len = MAX_BUF_SIZE;
                    continue;
                }

                applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, 
                        "receive xml string: %s\n", recv_buf);
                if (recv_len > 32 && memcmp(recv_buf + recv_len - 15, "</CALL_REQUEST>", 15) == 0) {
                    get_object_clue(recv_buf, &obj_clue);
                    get_online_call(&obj_clue);
                    send_online_call(ONLINE_XML_FILE, client_fd[i]);
                    node[i].recv_len = 0;
                    node[i].remain_len = MAX_BUF_SIZE;
                }
            }
        }
    }

    pthread_exit(NULL);
}
