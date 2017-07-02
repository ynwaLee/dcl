#include "mfusrv.h"
#include "common_header.h"
#include "sockop.h"
#include "svmtel.h"
#include "svmmob.h"
#include "svmvoipA.h"
#include "svmvoipB.h"
#include "macli.h"
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int encrypt_wave_header(uint8_t *data, uint32_t data_len)
{
    uint32_t i;

    for (i = 0; i < data_len; i++) {
        data[i ] = data[i] ^ 0x53;
    }

    return 0;
}


static mfusrv_para_t g_mfusrv_para;
mfusrv_para_t *mfusrv_para_get(void)
{
    return &g_mfusrv_para;
}

void mfusrv_para_destroy(mfusrv_para_t *para)
{
    ipport_t *curr;
    ipport_t *next;

    for (curr = para->svmtel_addr; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }
    for (curr = para->svmmob_addr; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }
    //shiwb
    for (curr = para->svmvoipA_addr; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }
    for (curr = para->svmvoipB_addr; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }
    
    para->svmtel_addr = NULL;
    para->svmmob_addr = NULL;
    para->svmvoipA_addr = NULL;
    para->svmvoipB_addr = NULL;
}

static int get_mfu_conf_path(int argc, char *argv[], char *mfu_conf_path)
{
    char *pv;
    
    snprintf(mfu_conf_path, MFU_CONF_PATH_LEN, "%s/%s", MFU_CONF_DIR, MFU_CONF_FILE);
    pv = get_arg_value_string(argc, argv, "--config");
    if (pv) {
        if (strlen(pv) > 127) {
            xapplog(LOG_WARNING, APP_LOG_MASK_CDR, "--config path is too long.");
        }else {
            snprintf(mfu_conf_path, MFU_CONF_PATH_LEN, "%s/%s", pv, MFU_CONF_FILE);
        }
    }

    return 0;
}
static int get_mfu_svmvoipB(mfusrv_para_t *para)//shiwb
{
    ConfNode *base;
    ConfNode *child;
    ipport_t *curr = NULL;
    ipport_t *tail = NULL;

    if ((base = ConfGetNode("voipB")) == NULL) {
        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "there is no voipB node in mfu.yaml");
        return -1;
    }

    TAILQ_FOREACH(child, &base->head, next) {
        char ip[16] = {0};
        unsigned short port = 0;

        if (strcmp(child->val, "id") == 0) {
            ConfNode *subchild;

            TAILQ_FOREACH(subchild, &child->head, next) {
                if (!(strcmp(subchild->name, "ip"))) {
                    strncpy(ip, subchild->val, 15);
                }

                if (!strcmp(subchild->name, "port")) {
                    port = atoi(subchild->val);
                }
            }

            if (ip[0] != 0 && port != 0) {
                curr = calloc(1, sizeof(ipport_t));
                curr->port = port;
                strcpy(curr->ip, ip);
                if (tail == NULL) {
                    para->svmvoipB_addr = curr;
                }else {
                    tail->next = curr;
                }
                tail = curr;
                xapplog(LOG_INFO, APP_LOG_MASK_BASE, "voipB ip: %s, port: %d", ip, port);
            }else {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Error config of voipB ip: %s, port: %d", ip, port);
            }
        }
    }
    return 0;
}

static int get_mfu_svmvoipA(mfusrv_para_t *para)//shiwb
{
    ConfNode *base;
    ConfNode *child;
    ipport_t *curr = NULL;
    ipport_t *tail = NULL;

    if ((base = ConfGetNode("voipA")) == NULL) {
        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "there is no voipA node in mfu.yaml");
        return -1;
    }

    TAILQ_FOREACH(child, &base->head, next) {
        char ip[16] = {0};
        unsigned short port = 0;
		char citycode[16] = {0};

        if (strcmp(child->val, "id") == 0) {
            ConfNode *subchild;

            TAILQ_FOREACH(subchild, &child->head, next) {
                if (!(strcmp(subchild->name, "ip"))) {
                    strncpy(ip, subchild->val, 15);
                }

                if (!strcmp(subchild->name, "port")) {
                    port = atoi(subchild->val);
                }
				if (!strcmp(subchild->name, "citycode")) {
                    strncpy(citycode, subchild->val, 15);
                }
            }

            if (ip[0] != 0 && port != 0 && citycode[0] != 0) {
                curr = calloc(1, sizeof(ipport_t));
                curr->port = port;
                strcpy(curr->ip, ip);
				strcpy(curr->city_code , citycode);
                if (tail == NULL) {
                    para->svmvoipA_addr = curr;
                }else {
                    tail->next = curr;
                }
                tail = curr;
                xapplog(LOG_INFO, APP_LOG_MASK_BASE, "voipA ip: %s, port: %d", ip, port);
            }else {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Error config of voipA ip: %s, port: %d", ip, port);
            }
        }
    }
    return 0;
}

static int get_mfu_svmtel(mfusrv_para_t *para)
{
    ConfNode *base;
    ConfNode *child;
    ipport_t *curr = NULL;
    ipport_t *tail = NULL;

    if ((base = ConfGetNode("svmtel")) == NULL) {
        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "there is no svmtel node in mfu.yaml");
        return -1;
    }

    TAILQ_FOREACH(child, &base->head, next) {
        char ip[16] = {0};
        unsigned short port = 0;

        if (strcmp(child->val, "id") == 0) {
            ConfNode *subchild;

            TAILQ_FOREACH(subchild, &child->head, next) {
                if (!(strcmp(subchild->name, "ip"))) {
                    strncpy(ip, subchild->val, 15);
                }

                if (!strcmp(subchild->name, "port")) {
                    port = atoi(subchild->val);
                }
            }

            if (ip[0] != 0 && port != 0) {
                curr = calloc(1, sizeof(ipport_t));
                curr->port = port;
                strcpy(curr->ip, ip);
                if (tail == NULL) {
                    para->svmtel_addr = curr;
                }else {
                    tail->next = curr;
                }
                tail = curr;
                xapplog(LOG_INFO, APP_LOG_MASK_BASE, "svmtel ip: %s, port: %d", ip, port);
            }else {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Error config of svmtel ip: %s, port: %d", ip, port);
            }
        }
    }
    return 0;
}
static int get_mfu_svmmob(mfusrv_para_t *para)
{
    ConfNode *base;
    ConfNode *child;
    ipport_t *curr = NULL;
    ipport_t *tail = NULL;

    if ((base = ConfGetNode("svmmob")) == NULL) {
        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "there is no svmmob node in mfu.yaml");
        return -1;
    }

    TAILQ_FOREACH(child, &base->head, next) {
        char ip[16] = {0};
        unsigned short port = 0;

        if (strcmp(child->val, "id") == 0) {
            ConfNode *subchild;

            TAILQ_FOREACH(subchild, &child->head, next) {
                if (!(strcmp(subchild->name, "ip"))) {
                    strncpy(ip, subchild->val, 15);
                }

                if (!strcmp(subchild->name, "port")) {
                    port = atoi(subchild->val);
                }
            }

            if (ip[0] != 0 && port != 0) {
                curr = calloc(1, sizeof(ipport_t));
                curr->port = port;
                strcpy(curr->ip, ip);
                if (tail == NULL) {
                    para->svmmob_addr = curr;
                }else {
                    tail->next = curr;
                }
                tail = curr;
                xapplog(LOG_INFO, APP_LOG_MASK_BASE, "svmmob ip: %s, port: %d", ip, port);
            }else {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Error config of svmmob ip: %s, port: %d", ip, port);
            }
        }
    }

    return 0;
}
static int get_mfu_conf(const char *path, mfusrv_para_t *para)
{
    char *value = NULL;

    ConfInit();

    if (ConfYamlLoadFile(path) != 0) {
        exit(EXIT_FAILURE);
    }
    
    if (ConfGet("mfu.ip", &value) == 1) {
        strcpy(para->mfuaddr.ip, value);
    }
    
    if (ConfGet("mfu.port", &value) == 1) {
        para->mfuaddr.port = atoi(value);
    }
    xapplog(LOG_INFO, APP_LOG_MASK_CDR, "mfu server ip: %s, port: %d", para->mfuaddr.ip, para->mfuaddr.port);

    if (get_mfu_svmtel(para) == -1) {
        mfusrv_para_destroy(para);
        ConfDeInit();
        return -1;
    }

    if (get_mfu_svmmob(para) == -1) {
        mfusrv_para_destroy(para);
        ConfDeInit();
        return -1;
    }
    //shiwb
    if (get_mfu_svmvoipA(para) == -1) {
        mfusrv_para_destroy(para);
        ConfDeInit();
        return -1;
    }
    //shiwb
    if (get_mfu_svmvoipB(para) == -1) {
        mfusrv_para_destroy(para);
        ConfDeInit();
        return -1;
    }
    ConfDeInit();
    return 0;
}

int mfusrv_para_init(mfusrv_para_t *mfusrvpara, cmdline_para_t *cmdlpara)
{
    char mfu_conf_path[MFU_CONF_PATH_LEN] = {0};
    get_mfu_conf_path(cmdlpara->argc, cmdlpara->argv, mfu_conf_path);
    mfu_mutex_lock();
    get_mfu_conf(mfu_conf_path, mfusrvpara);
    mfu_mutex_unlock();
    return 0;
}

static void mfusrv_deal_request(int listenfd)
{
    char *systype_key = "systype";
    char *systype_val;
    struct sockaddr_in peeraddr;
    socklen_t peerlen = sizeof(peeraddr);

    int sockfd = accept(listenfd, (struct sockaddr *)&peeraddr, &peerlen);
    if (sockfd == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "accept error.");
        return;
    }
    //xapplog(LOG_INFO, APP_LOG_MASK_BASE, "%s %d connected.", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));

    atomic_add(&g_mfu_counter.media_req_total, 1);
    char buf[MAX_HTTP_HEADER_LEN] = {0};
    int ret = recv(sockfd, buf, MAX_HTTP_HEADER_LEN - 1, MSG_PEEK);
    xapplog(LOG_ERR, APP_LOG_MASK_BASE, "MSG PEEK: %s.", buf);
    if (ret <= 0) {
        atomic_add(&g_mfu_counter.media_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "faled to recv http request header.");
        close(sockfd);
        return;
    }

    if ((systype_val = strstr(buf, systype_key)) == NULL) {
        atomic_add(&g_mfu_counter.media_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "can't find 'systype' in http request header.");
        close(sockfd);
        return;
    }

    systype_val += strlen(systype_key) + 1;
    char vbuf[100] = {0};
    int i = 0;

    while (isdigit(*systype_val)) {
        vbuf[i++] = *systype_val;
        systype_val++;
    }
    if (vbuf[0] == 0) {
        atomic_add(&g_mfu_counter.media_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "the systype's value is null.");
        close(sockfd);
        return;
    }
    int value = atoi(vbuf);

    pthread_t tid;
    switch (value) {
        case SVMTELPHONE:
            if (pthread_create(&tid, NULL, svmtel_thread, (void*)(unsigned long)sockfd) == -1) {
                atomic_add(&g_mfu_counter.media_req_invalid, 1);
                close(sockfd);
                return;
            }
            pthread_detach(tid);
            break;
        case SVMMOBILE:
            if (pthread_create(&tid, NULL, svmmob_thread, (void*)(unsigned long)sockfd) == -1) {
                atomic_add(&g_mfu_counter.media_req_invalid, 1);
                close(sockfd);
                return;
            }
            pthread_detach(tid);
            break;
        case SVMVOIPA://shiwb
            if (pthread_create(&tid, NULL, svmvoipA_thread, (void*)(unsigned long)sockfd) == -1) {
                atomic_add(&g_mfu_counter.media_req_invalid, 1);
                close(sockfd);
                return;
            }
            pthread_detach(tid);
            break;
        case SVMVOIPB://shiwb
            if (pthread_create(&tid, NULL, svmvoipB_thread, (void*)(unsigned long)sockfd) == -1) {
                atomic_add(&g_mfu_counter.media_req_invalid, 1);
                close(sockfd);
                return;
            }
            pthread_detach(tid);
            break;
        default:
            atomic_add(&g_mfu_counter.media_req_invalid, 1);
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "can't recongnize the systype: %d", value);
            close(sockfd);
            return;
    }
    atomic_add(&g_mfu_counter.media_req_valid, 1);

    return;
}

void *mfusrv_thread(void *arg)
{
    cmdline_para_t *cmdlpara = (cmdline_para_t *)arg;
    mfusrv_para_t *mfusrvpara = &g_mfusrv_para;
    
    int done = 0;
    bool mfusrv_para_init_flag = false;
    bool listen_flag = false;
    int reload_flag = 0;
    int listenfd = 0;
    char oldip[16] = {0};
    unsigned short oldport = 0;
    while (!done) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }

        if (!mfusrv_para_init_flag) {
            if (mfusrv_para_init(mfusrvpara, cmdlpara) == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to init mfusrv para.");
                continue;
            }
            mfusrv_para_init_flag = true;
        }
        
        reload_flag = get_reload_flag();
        if (reload_flag & CONF_RELOAD_MFU) {
            mfusrv_para_destroy(mfusrvpara);
            mfusrv_para_init_flag = false;

            reload_flag &= ~CONF_RELOAD_MFU;
            set_reload_flag(reload_flag);
            continue;
        }
        
        if (!listen_flag) {
            if ((listenfd = sock_listen(mfusrvpara->mfuaddr.ip, mfusrvpara->mfuaddr.port)) == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to listen");
                sleep(3);
                continue;
            }
            strcpy(oldip, mfusrvpara->mfuaddr.ip);
            oldport = mfusrvpara->mfuaddr.port;
            listen_flag = true;
        }

        if (oldport != mfusrvpara->mfuaddr.port || strcmp(mfusrvpara->mfuaddr.ip, oldip) != 0) {
            close(listenfd);
            listen_flag = false;
            continue;
        }

        
        if (sock_readable(listenfd, 1, 0)) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "start listen ok");
            mfusrv_deal_request(listenfd);
        }
    }

    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "%s", "mfusrv is done.");
    close(listenfd);
    return NULL;
}
