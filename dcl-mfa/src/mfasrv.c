#include "mfasrv.h"
#include "ftransfer.h"
#include "common_header.h"
#include "sockop.h"
#include "macli.h"
#include <stdlib.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

static mfasrv_para_t g_mfasrv_para;
static online_list_t g_online_list;

unsigned long long g_mfasrv_reload_time = 0;

#if 0

static int get_mfa_conf(const char *path, mfasrv_para_t *para)
{
    //char *value = NULL;
    ConfInit();

    if (ConfYamlLoadFile(path) != 0) {
        exit(EXIT_FAILURE);
    }
/*
    if (ConfGet("mfa.ip", &value) == 1) {
        strcpy(para->svmmobladdr.ip, value);
    }
    
    if (ConfGet("mfa.port", &value) == 1) {
        para->svmmobladdr.port = atoi(value);
    }
*/    
    ConfNode *base;
    ConfNode *child;

    base = ConfGetNode("paths");
    if (base == NULL) {
        return -1;
    }

    path_t *new = NULL;
    path_t *tail = para->pathmoblhdr;

    TAILQ_FOREACH(child, &base->head, next) {
        if (!strcmp(child->val, "id")) {
            ConfNode *subchild;
            TAILQ_FOREACH(subchild, &child->head, next) {
                if ((!strcmp(subchild->name, "id"))) {
                    if ((subchild->val == NULL)||(subchild->val[0] == 0)) {
                        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad subchild value.");
                        return -1;
                    }
                    new = calloc(1, sizeof(path_t));
                    new->id = atoi(subchild->val);

                    if (tail == NULL) {
                        para->pathmoblhdr = new;
                    }else {
                        tail->next = new;
                    }
                    tail = new;
                }

                if ((!strcmp(subchild->name, "path"))) {
                    if ((subchild->val == NULL)||(subchild->val[0] == 0)) {
                        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad subchild value.");
                        return -1;
                    }
                    snprintf(tail->path, MFA_PATH_LEN, "%s", subchild->val);
                }
            }
        }
    }

    xapplog(LOG_INFO, APP_LOG_MASK_CDR, "mfa server ip: %s, port: %d", para->svmmobladdr.ip, para->svmmobladdr.port);
    for (tail = para->pathmoblhdr; tail != NULL; tail = tail->next) {
        xapplog(LOG_INFO, APP_LOG_MASK_CDR, "%d: %s", tail->id, tail->path);
    }

    ConfDeInit();
    return 0;
}


#endif
inline online_list_t *online_list_get(void)
{
    return &g_online_list;
}
inline mfasrv_para_t *mfasrv_para_get(void)
{
    return &g_mfasrv_para;
}

static int get_mfa_conf_path(int argc, char *argv[], char *mfa_conf_path)
{
    char *pv;
    
    snprintf(mfa_conf_path, MFA_CONF_PATH_LEN, "%s/%s", MFA_CONF_DIR, MFA_CONF_FILE);
    pv = get_arg_value_string(argc, argv, "--config");
    if (pv) {
        if (strlen(pv) > 127) {
            xapplog(LOG_WARNING, APP_LOG_MASK_CDR, "--config path is too long.");
        }else {
            snprintf(mfa_conf_path, MFA_CONF_PATH_LEN, "%s/%s", pv, MFA_CONF_FILE);
        }
    }

    return 0;
}
static int get_mfa_svmmobaddr(mfasrv_para_t *para)
{
    ConfNode *base;
    ConfNode *child;
    ipport_t *curr = NULL;
    ipport_t *tail = NULL;

    if ((base = ConfGetNode("svmmob")) == NULL) {
        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "there is no svmmob node in mfa.yaml");
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
                    para->svmmobladdr= curr;
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
static int get_mfa_voipAaddr(mfasrv_para_t *para)
{
    ConfNode *base;
    ConfNode *child;
    ipport_t *curr = NULL;
    ipport_t *tail = NULL;

    if ((base = ConfGetNode("voipA")) == NULL) {
        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "there is no voipA node in mfa.yaml");
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
                    para->voipAaddr = curr;
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


static int get_mfa_voipBaddr(mfasrv_para_t *para)
{
    ConfNode *base;
    ConfNode *child;
    ipport_t *curr = NULL;
    ipport_t *tail = NULL;

    if ((base = ConfGetNode("voipB")) == NULL) {
        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "there is no voipA node in mfa.yaml");
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
                    para->voipBaddr = curr;
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

static int get_svmmobpath(mfasrv_para_t *para)
{
    ConfNode *child;
    ConfNode *base = ConfGetNode("pathsmobl");
    path_t *new = NULL;
    path_t *tail = para->pathmoblhdr;
    //path_t *tail = NULL;
    if (base == NULL) {
        return -1;
    }
    TAILQ_FOREACH(child, &base->head, next) {
        if (!strcmp(child->val, "id")) {
            ConfNode *subchild;
            TAILQ_FOREACH(subchild, &child->head, next) {
                if ((!strcmp(subchild->name, "id"))) {
                    if ((subchild->val == NULL)||(subchild->val[0] == 0)) {
                        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad subchild value.");
                        return -1;
                    }
                    new = calloc(1, sizeof(path_t));
                    new->id = atoi(subchild->val);

                    if (tail == NULL) {
                        para->pathmoblhdr = new;
                    }else {
                        tail->next = new;
                    }
                    tail = new;
                }

                if ((!strcmp(subchild->name, "path"))) {
                    if ((subchild->val == NULL)||(subchild->val[0] == 0)) {
                        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad subchild value.");
                        return -1;
                    }
                    snprintf(tail->path, MFA_PATH_LEN, "%s", subchild->val);
                }
            }
        }
    }

    xapplog(LOG_INFO, APP_LOG_MASK_CDR, "mfa server ip: %s, port: %d", para->svmmobladdr->ip, para->svmmobladdr->port);
    for (tail = para->pathmoblhdr; tail != NULL; tail = tail->next) {
        xapplog(LOG_INFO, APP_LOG_MASK_CDR, "%d: %s", tail->id, tail->path);
    }
    
  //  ConfDeInit();
    return 0;

}


static int get_voipApath(mfasrv_para_t *para)
{
    
    ConfNode *child;
    ConfNode *base = ConfGetNode("pathsvoipa");    
    path_t *new = NULL;
    path_t *tail = para->pathvoipAhdr;
    //path_t *tail = NULL;
    if (base == NULL) {
        return -1;
    }
    TAILQ_FOREACH(child, &base->head, next) {
        if (!strcmp(child->val, "id")) {
            ConfNode *subchild;
            TAILQ_FOREACH(subchild, &child->head, next) {
                if ((!strcmp(subchild->name, "id"))) {
                    if ((subchild->val == NULL)||(subchild->val[0] == 0)) {
                        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad subchild value.");
                        return -1;
                    }
                    new = calloc(1, sizeof(path_t));
                    new->id = atoi(subchild->val);

                    if (tail == NULL) {
                        para->pathvoipAhdr = new;
                    }else {
                        tail->next = new;
                    }
                    tail = new;
                }

                if ((!strcmp(subchild->name, "path"))) {
                    if ((subchild->val == NULL)||(subchild->val[0] == 0)) {
                        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad subchild value.");
                        return -1;
                    }
                    snprintf(tail->path, MFA_PATH_LEN, "%s", subchild->val);
                }
            }
        }
    }
    
    xapplog(LOG_INFO, APP_LOG_MASK_CDR, "mfa server ip: %s, port: %d", para->voipAaddr->ip, para->voipAaddr->port);
    for (tail = para->pathvoipAhdr; tail != NULL; tail = tail->next) {
        xapplog(LOG_INFO, APP_LOG_MASK_CDR, "%d: %s", tail->id, tail->path);
    }
   // ConfDeInit();

    return 0;

}


static int get_voipBpath(mfasrv_para_t *para)
{
    ConfNode *child;
    ConfNode *base = ConfGetNode("pathsvoipb");
    path_t *new = NULL;
    path_t *tail = para->pathvoipBhdr;
    //path_t *tail = NULL;
    if (base == NULL) {
        return -1;
    }
    TAILQ_FOREACH(child, &base->head, next) {
        if (!strcmp(child->val, "id")) {
            ConfNode *subchild;
            TAILQ_FOREACH(subchild, &child->head, next) {
                if ((!strcmp(subchild->name, "id"))) {
                    if ((subchild->val == NULL)||(subchild->val[0] == 0)) {
                        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad subchild value.");
                        return -1;
                    }
                    new = calloc(1, sizeof(path_t));
                    new->id = atoi(subchild->val);

                    if (tail == NULL) {
                        para->pathvoipBhdr = new;
                    }else {
                        tail->next = new;
                    }
                    tail = new;
                }

                if ((!strcmp(subchild->name, "path"))) {
                    if ((subchild->val == NULL)||(subchild->val[0] == 0)) {
                        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad subchild value.");
                        return -1;
                    }
                    snprintf(tail->path, MFA_PATH_LEN, "%s", subchild->val);
                }
            }
        }
    }

    xapplog(LOG_INFO, APP_LOG_MASK_CDR, "mfa server ip: %s, port: %d", para->voipAaddr->ip, para->voipAaddr->port);
    for (tail = para->pathvoipBhdr; tail != NULL; tail = tail->next) {
        xapplog(LOG_INFO, APP_LOG_MASK_CDR, "%d: %s", tail->id, tail->path);
    }

    ConfDeInit();
    return 0;

}

static int get_mfa_conf(const char *path, mfasrv_para_t *para)
{
    //char *value = NULL;
    ConfInit();

    if (ConfYamlLoadFile(path) != 0) {
        exit(EXIT_FAILURE);
    }
    if ( get_mfa_svmmobaddr(para) != 0){
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "get mfa svmmobaddr error");
    }

    if ( get_mfa_voipAaddr(para) != 0){
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "get mfa voipAaddr error");
    }
    if ( get_mfa_voipBaddr(para) != 0){
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "get mfa voipBaddr error");
    }
    if ( get_svmmobpath(para) != 0){
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "get svmmobpath error");
    }
    if ( get_voipApath(para) != 0){
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "get voipApath error");
    }
    if ( get_voipBpath(para) != 0){
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "get voipBpath error");
    }
    return 0;
}

int mfasrv_para_init(mfasrv_para_t *mfasrvpara, cmdline_para_t *cmdlpara)
{
    char mfa_conf_path[MFA_CONF_PATH_LEN] = {0};
    get_mfa_conf_path(cmdlpara->argc, cmdlpara->argv, mfa_conf_path);
    mfa_mutex_lock();
    get_mfa_conf(mfa_conf_path, mfasrvpara);
    mfa_mutex_unlock();
    return 0;
}
//mfasrv_para_clear_unlock pathmoblhdr //Ö»ÊÇ¶Ôyj¼ÏËø »¹ÓÐÆäËûµÄÒ²Òª¼ÏËøvoipA voipB
int mfasrv_para_clear_unlock(mfasrv_para_t *mfasrvpara)
{
    path_t *curr = NULL;
    path_t *next = NULL;
    for (curr = mfasrvpara->pathmoblhdr; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }
    memset(mfasrvpara, 0, sizeof(mfasrv_para_t));
    return 0;
}

//mfasrv_para_clear_lock pathmoblhdr //Ö»ÊÇ¶Ôyj¼ÏËø »¹ÓÐÆäËûµÄÒ²Òª¼ÏËøvoipA voipB
int mfasrv_para_clear_lock(mfasrv_para_t *mfasrvpara)
{
    mfa_mutex_lock();
    path_t *curr = NULL;
    path_t *next = NULL;
    for (curr = mfasrvpara->pathmoblhdr; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }
    #if 1
    for (curr = mfasrvpara->pathvoipAhdr; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }
    for (curr = mfasrvpara->pathvoipBhdr; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }
    #endif
    memset(mfasrvpara, 0, sizeof(mfasrv_para_t));
    mfa_mutex_unlock();
    return 0;
}

static int mfasrv_deal_request(int listenfd,int systypes)
{
    pthread_t ftransfer_tid;
    ipportsockfd *ipport = (ipportsockfd *)malloc(sizeof(ipportsockfd ));
    int sockfd = accept(listenfd, NULL, NULL);
    if (sockfd == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "accept error.");
        return -1;
    }
    if(ipport == NULL)
    {    xapplog(LOG_ERR, APP_LOG_MASK_BASE, "malloc  ipport  error contuine");
        ipport = (ipportsockfd *)malloc(sizeof(ipportsockfd ));
    }
    ipport->socketfd =sockfd;
    ipport->systypes =systypes;
    if (pthread_create(&ftransfer_tid, NULL, ftransfer_thread, (void *)(unsigned long)ipport) == -1) {
        close(sockfd);
    }
    return 0;
}

void *mfasrv_thread(void *arg)
{
    //åˆå§‹åŒ–g729è§£ç åº“çš„é”
    extern pthread_mutex_t g_g729_decoder_mutex;
    pthread_mutex_init(&g_g729_decoder_mutex, NULL);
    
    cmdline_para_t *cmdlpara = (cmdline_para_t *)arg;
    mfasrv_para_t *mfasrvpara = &g_mfasrv_para;
    online_list_init(&g_online_list);
    
    int done = 0;
    bool mfasrv_para_init_flag = false;
    bool listen_flag[3] = {false};
    int reload_flag = 0;
    int listenfd[3] = {0};
    char oldip[16] = {'0'};
    unsigned short oldport[3] = {0};
    while (!done) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }

        if (!mfasrv_para_init_flag) {
            if (mfasrv_para_init(mfasrvpara, cmdlpara) == -1) {
                mfasrv_para_clear_lock(mfasrvpara);
                continue;
            }
            mfasrv_para_init_flag = true;
        }
        
        reload_flag = get_reload_flag();
        if (reload_flag & CONF_RELOAD_MFA) {
            mfasrv_para_clear_lock(mfasrvpara);
            mfasrv_para_init_flag = false;

            reload_flag &= ~CONF_RELOAD_MFA;
            set_reload_flag(reload_flag);
            g_mfasrv_reload_time++;
            continue;
        }
        
        if (!listen_flag[0]) {
            if ((listenfd[0] = sock_listen(mfasrvpara->svmmobladdr->ip, mfasrvpara->svmmobladdr->port)) == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "listen error %s, ip: %s, port: %d", strerror(errno), mfasrvpara->svmmobladdr->ip, mfasrvpara->svmmobladdr->port);
                sleep(1);
                continue;
            }
            xapplog(LOG_INFO, APP_LOG_MASK_BASE, " ip: %s port: %d, previous ip: %s, port %d.",
                    mfasrvpara->svmmobladdr->ip, mfasrvpara->svmmobladdr->port, oldip, oldport[0]);
            strcpy(oldip, mfasrvpara->svmmobladdr->ip);
            oldport[0] = mfasrvpara->svmmobladdr->port;
            listen_flag[0] = true;
        }

        if (oldport[0] != mfasrvpara->svmmobladdr->port || strcmp(mfasrvpara->svmmobladdr->ip, oldip) != 0) {
            xapplog(LOG_INFO, APP_LOG_MASK_BASE, "rebind ip: %s port: %d, previous ip: %s, port %d.",
                    mfasrvpara->svmmobladdr->ip, mfasrvpara->svmmobladdr->port, oldip, oldport[0]);
            if (listen_flag[0] == true) {
                close(listenfd[0]);
                listen_flag[0] = false;
                continue;
            }
        }
#if 1
        if (!listen_flag[1]) {
            if ((listenfd[1] = sock_listen(mfasrvpara->voipAaddr->ip, mfasrvpara->voipAaddr->port)) == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "listen error %s, ip: %s, port: %d", strerror(errno), mfasrvpara->voipAaddr->ip, mfasrvpara->voipAaddr->port);
                sleep(1);
                continue;
            }
            strcpy(oldip, mfasrvpara->voipAaddr->ip);
            oldport[1] = mfasrvpara->voipAaddr->port;
            listen_flag[1] = true;
        }

        if (oldport[1] != mfasrvpara->voipAaddr->port || strcmp(mfasrvpara->voipAaddr->ip, oldip) != 0) {
            xapplog(LOG_INFO, APP_LOG_MASK_BASE, "rebind ip: %s port: %d, previous ip: %s, port %d.",
                    mfasrvpara->voipAaddr->ip, mfasrvpara->voipAaddr->port, oldip, oldport[1]);
            if (listen_flag[1] == true) {
                close(listenfd[1]);
                listen_flag[1] = false;
                continue;
            }
        }

        if (!listen_flag[2]) {
            if ((listenfd[2] = sock_listen(mfasrvpara->voipBaddr->ip, mfasrvpara->voipBaddr->port)) == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "listen error %s, ip: %s port: %d", strerror(errno), mfasrvpara->voipBaddr->ip, mfasrvpara->voipBaddr->port);
                sleep(1);
                continue;
            }
            strcpy(oldip, mfasrvpara->voipBaddr->ip);
            oldport[2] = mfasrvpara->voipBaddr->port;
            listen_flag[2] = true;
        }

        if (oldport[2] != mfasrvpara->voipBaddr->port || strcmp(mfasrvpara->voipBaddr->ip, oldip) != 0) {
            xapplog(LOG_INFO, APP_LOG_MASK_BASE, "rebind ip: %s port: %d, previous ip: %s, port %d.",
                    mfasrvpara->voipBaddr->ip, mfasrvpara->voipBaddr->port, oldip, oldport[2]);
            if (listen_flag[2] == true) {
                close(listenfd[2]);
                listen_flag[2] = false;
                continue;
            }
        }
#endif

        if (sock_readable(listenfd[0], 1, 0)) {
            mfasrv_deal_request(listenfd[0],2);    
        }
#if 1    
        if (sock_readable(listenfd[1], 1, 0)) {
            mfasrv_deal_request(listenfd[1],3);    
        }
        if (sock_readable(listenfd[2], 1, 0)) {
            mfasrv_deal_request(listenfd[2],4);    
        }
#endif
    }
    
    mfasrv_para_clear_lock(mfasrvpara);
    online_list_destroy(&g_online_list);
    close(listenfd[0]);
    close(listenfd[1]);
    close(listenfd[2]);
    pthread_mutex_destroy(&g_g729_decoder_mutex);
    return NULL;
}
