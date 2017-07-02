#include "svmtel.h"
#include "mfusrv.h"
#include "mfu.h"
#include "sockop.h"
#include "http_reqpara.h"
#include "common_header.h"
#include <arpa/inet.h>

#if 0
static void wav_header_init_couple(struct wav_header *header)
{
    memcpy(header->riff, "RIFF", 4);
    header->file_size = 0;
    memcpy(header->riff_type, "WAVE", 4);

    memcpy(header->fmt, "fmt ", 4);
    header->fmt_size = 16;
    header->fmt_tag = 6;
    header->fmt_channel = 2;
    header->fmt_samples_persec = 8000;
    header->avg_bytes_persec = 16000;
    header->block_align = 2;
    header->bits_persample = 8;

    memcpy(header->data, "data", 4);
    header->data_size = 0;

    return;
}

static void wav_header_init_single(struct wav_header *header)
{
    memcpy(header->riff, "RIFF", 4);
    header->file_size = 0;
    memcpy(header->riff_type, "WAVE", 4);

    memcpy(header->fmt, "fmt ", 4);
    header->fmt_size = 16;
    header->fmt_tag = 6;
    header->fmt_channel = 1;
    header->fmt_samples_persec = 8000;
    header->avg_bytes_persec = 8000;
    header->block_align = 1;
    header->bits_persample = 8;

    memcpy(header->data, "data", 4);
    header->data_size = 0;

    return;
}
#endif

static int pack_svmtel_req(svmtel_req_t *req, char *buf)
{
    uint16_t *tmp16;
    uint64_t *tmp64;

    //request for play
    tmp16 = (uint16_t *)buf;
    *tmp16 = htons(0x01);
    
    //total length
    tmp16 = (uint16_t *)(buf + 2);
    *tmp16 = htons(36);

    //callid type
    tmp16 = (uint16_t *)(buf + 4);
    *tmp16 = htons(0x01);
    
    tmp16 = (uint16_t *)(buf + 6);
    *tmp16 = htons(0x08);

    tmp64 = (uint64_t *)(buf + 8);
    *tmp64 = (req->callid);
    
    
    //case type
    tmp16 = (uint16_t *)(buf + 16);
    *tmp16 = htons(0x02);

    tmp16 = (uint16_t *)(buf + 18);
    *tmp16 = htons(0x02);

    if (req->scase == 0 || req->scase == 1) {
        tmp16 = (uint16_t *)(buf + 20);
        *tmp16 = htons(req->scase);
    }else {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "scase is error : %d", req->scase);
        return -1;
    }

    //real time type
    tmp16 = (uint16_t *)(buf + 22);
    *tmp16 = htons(0x03);
    
    tmp16 = (uint16_t *)(buf + 24);
    *tmp16 = htons(0x02);

    if (req->stime == 0 || req->stime == 1) {
        tmp16 = (uint16_t *)(buf + 26);
        *tmp16 = htons(req->stime);
    }else {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "stime is error : %d", req->stime);
        return -1;
    }

    //callflag type
    tmp16 = (uint16_t*)(buf + 28);
    *tmp16 = htons(0x04);
    
    tmp16 = (uint16_t *)(buf + 30);
    *tmp16 = htons(0x02);
    
    if (req->callflag == 0 || req->callflag == 1) {
        tmp16 = (uint16_t *)(buf + 32);
        *tmp16 = htons(req->callflag);
    }else {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "callflag is error : %d", req->scase);
        return -1;
    }

    //channel type
    tmp16 = (uint16_t*)(buf + 34);
    *tmp16 = htons(0x05);
    //len
    tmp16 = (uint16_t *)(buf + 36);
    *tmp16 = htons(0x02);
    //data
    tmp16 = (uint16_t *)(buf + 38);
    *tmp16 = htons(req->channel);

    return 40;
}


static int transfer_data_stream(int svmtelfd, int sockfd, svmtel_req_t *req)
{
    int recved, sended;
    char recvbuf[8] = {0};
    char sendbuf[4096] = {0};
    short *pshort = NULL;
    unsigned int *puint = NULL;
    short restype;
    short reslen;
    unsigned int filesize;
    int len;

    if (sock_recv(svmtelfd, recvbuf, 8, &recved) <= 0) {
        return -1;
    }
    
    pshort = (short *)recvbuf;
    restype = ntohs(*pshort);
    pshort = (short *)(recvbuf + 2);
    reslen  = ntohs(*pshort);
    puint = (unsigned int *)(recvbuf + 4);
    filesize  = ntohl(*puint);
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "type: %x, len: %x, filesize: %x", restype, reslen, filesize);

    if (req->callflag == CALL_FLAG_FAX) {
        if (restype != 0x04 || reslen != 0x04) {
            return -1;
        }
    }
    else if (req->callflag == CALL_FLAG_VOICE) {
        if (restype != 0x03 || reslen != 0x04) {
            return -1;
        }
    }
        
    len = snprintf((char *)sendbuf, 2048, "%s%s%s%s%s%u%s%s%s%s",
            HTTP_HEADER_CODE,
            HTTP_HEADER_SERVER,
            HTTP_HEADER_DATE,
            HTTP_HEADER_TYPE,
            "Content-Length: ",
            filesize,
            "\r\n",
            HTTP_HEADER_CONN,
            HTTP_HEADER_ACCEPT,
            HTTP_HEADER_END);
    if (sock_send(sockfd, sendbuf, len, &sended) == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send http header to media error.");
        return -1;
    }
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "response http header to media: %s", sendbuf);
    
    unsigned int recved_len = 0;
    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }
        
        char sendbuf[2048] = {0};
        fd_set rfds;
        struct timeval tv;
        int ret;
        int maxfd = (svmtelfd > sockfd) ? svmtelfd : sockfd;

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(svmtelfd, &rfds);
        FD_SET(sockfd, &rfds);
        
        ret = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(svmtelfd, &rfds)) {
            len = recv(svmtelfd, sendbuf, 2048, 0);
            if (len == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Failed to recv data from svmtel.");
                return -1;
            }
            if (len == 0) {
                xapplog(LOG_INFO, 
                        APP_LOG_MASK_BASE, 
                        "transfer the data successful filesize: %u, recved_len: %u", 
                        filesize, 
                        recved_len);
                return 0;
            }
            
            recved_len += len;
            ret = sock_send(sockfd, sendbuf, len, &sended);
            if (ret == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Failed to send data to media.");
                return -1;
            }
        }

        if (ret > 0 && FD_ISSET(sockfd, &rfds)) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Media closed the link.");
            return -1;
        }
    }

    return 0;
}

static int get_valid_svmtelfd(ipport_t *svmtel_addr, char *svmtel_reqbuf, int len)
{
    int sended;
    short *result = NULL;
    ipport_t *addr = svmtel_addr;

    for (;addr != NULL; addr = addr->next) {
        int tmpfd = sock_connect(addr->ip, addr->port);
        if (tmpfd == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "connect to %s, %d failed.", addr->ip, addr->port);
            continue;
        }
        if (sock_send(tmpfd, svmtel_reqbuf, len, &sended) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send request to %s, %d failed.", addr->ip, addr->port);
            close(tmpfd);
            continue;
        }

        char recvbuf[6] = {0};
        if (sock_recv(tmpfd, recvbuf, sizeof(recvbuf), &sended) <= 0) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "recv response from %s, %d failed.", addr->ip, addr->port);
            close(tmpfd);
            continue;
        }

        result = (short *)(recvbuf + 4);
        short ivalue = ntohs(*result);
        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "ip: %s, port: %d, return: %d", addr->ip, addr->port, ivalue);
        
        if (ivalue == 1) {
            xapplog(LOG_INFO, APP_LOG_MASK_BASE, "successful: ip: %s, port: %d find the data stream.", addr->ip, addr->port);
            return tmpfd;
        }
        close(tmpfd);
    }

    xapplog(LOG_ERR, APP_LOG_MASK_BASE, "can't find the data stream.");
    return -1;
}

void *svmtel_thread(void *arg)
{
    int ret;
    int sockfd = (int)(unsigned long)arg;
    mfusrv_para_t *para = mfusrv_para_get();
    ipport_t *svmtel_addr = para->svmtel_addr;
    

    atomic_add(&g_mfu_counter.svmtel_req_total, 1);
    char buf[MAX_HTTP_HEADER_LEN] = {0};
    ret = recv(sockfd, buf, MAX_HTTP_HEADER_LEN - 1, 0);
    if (ret <= 0) {
        atomic_add(&g_mfu_counter.svmtel_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to recv media request.");
        close(sockfd);
        return NULL;
    }

    char parabuf[MAX_HTTP_HEADER_LEN] = {0};
    if (http_reqpara_get(parabuf, buf) == -1) {
        atomic_add(&g_mfu_counter.svmtel_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to get http request para.");
        close(sockfd);
        return NULL;
    }
    
    http_reqpara_t reqpara = {0};
    if (http_reqpara_parse(parabuf, &reqpara) == -1) {
        atomic_add(&g_mfu_counter.svmtel_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to parse http para.");
        close(sockfd);
        return NULL;
    }
    http_reqpara_show(&reqpara);    
    atomic_add(&g_mfu_counter.svmtel_req_valid, 1);

    atomic_add(&g_mfu_counter.svmtel_res_total, 1);
    svmtel_req_t svmtel_req = {0};
    svmtel_req.callid = reqpara.callid;
    svmtel_req.scase = (reqpara.clueid == 0) ? 0 : 1;
    svmtel_req.stime = (reqpara.online == 1) ? 1 : 0;
    svmtel_req.callflag = (reqpara.callflag == 1) ? 1 : 0;
    svmtel_req.channel = reqpara.channel;
    char svmtel_reqbuf[128] = {0};
    int len;
    if ((len = pack_svmtel_req(&svmtel_req, svmtel_reqbuf)) == -1) {
        atomic_add(&g_mfu_counter.svmtel_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed pack svmtel request.");
        close(sockfd);
        return NULL;
    }
    
    int svmtelfd = 0;
    if ((svmtelfd = get_valid_svmtelfd(svmtel_addr, svmtel_reqbuf, len)) == -1) {
        //HTTP/1.1 404 Not Found 
        int sended;
        if (sock_send(sockfd, HTTP_404_HEADER, sizeof(HTTP_404_HEADER) - 1, &sended) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send http 404 header to media error.");
        }
        atomic_add(&g_mfu_counter.svmtel_res_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to get valid svmtel fd.");
        close(sockfd);
        return NULL;
    }
    
    if ((ret = transfer_data_stream(svmtelfd, sockfd, &svmtel_req)) == -1) {
        atomic_add(&g_mfu_counter.svmtel_res_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to transfer data stream.");
        close(sockfd);
        close(svmtelfd);
        return NULL;
    }

    close(svmtelfd);
    close(sockfd);
    atomic_add(&g_mfu_counter.svmtel_res_valid, 1);
    atomic_add(&g_mfu_counter.media_res_valid, 1);
    return NULL;
}
