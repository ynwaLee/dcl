#include "svmvoipB.h"
#include "mfu.h"
#include "mfusrv.h"
#include "common_header.h"
#include "http_reqpara.h"
#include "sockop.h"
#include <strings.h>
#include <fcntl.h>
#include <errno.h>



static int valcopy_voipB(unsigned long srcval, int size, void *dstbuf, int offset, int flag)
{
    if (flag == 0) {                                     
        unsigned int val = (unsigned int)srcval;       
        bcopy(&val, dstbuf + offset, size);              
    }else if (flag == 1){                                
        void *str = (void *)srcval;
        bcopy(str, dstbuf + offset, size);              
    }                                                    
    return offset + size;                                       
}

static int pack_svmvoipB_req(char *buf, http_reqpara_t *reqpara) 
{
    //unsigned short type = 0;
    unsigned short offset = 0;

    offset = 0;
    //type len
    offset = valcopy_voipB(0x10, 1, buf, offset, 0);
    offset = valcopy_voipB(0x00, 2, buf, offset, 0);
#if 1
    //systype  
    offset = valcopy_voipB(0x02, 2, buf, offset, 0);
    offset = valcopy_voipB(0x04, 2, buf, offset, 0);
    offset = valcopy_voipB(reqpara->systype, 4, buf, offset, 0);
#endif

    //type len online
    offset = valcopy_voipB(0x00, 2, buf, offset, 0);
    offset = valcopy_voipB(0x04, 2, buf, offset, 0);
    offset = valcopy_voipB(reqpara->online, 4, buf, offset, 0);

    //type len mntid
    offset = valcopy_voipB(0x01, 2, buf, offset, 0);
    offset = valcopy_voipB(0x08, 2, buf, offset, 0);
    offset = valcopy_voipB(reqpara->clueid, 8, buf, offset, 0);

    //type len channel
    offset = valcopy_voipB(0x02, 2, buf, offset, 0);
    offset = valcopy_voipB(0x04, 2, buf, offset, 0);
    offset = valcopy_voipB(reqpara->callcode, 4, buf, offset, 0);

    //type len vocfile
    unsigned int filesize = strlen(reqpara->vocfile) + 1;
    offset = valcopy_voipB(0x03, 2, buf, offset, 0);
    offset = valcopy_voipB(filesize, 2, buf, offset, 0);
    offset = valcopy_voipB((unsigned long)reqpara->vocfile, filesize, buf, offset, 1);

    unsigned int len = offset - 3;
    valcopy_voipB(len, 2, buf, 1, 0);

    return offset;
}



static int get_valid_svmvoipBfd(ipport_t *svmvoipB_addr, http_reqpara_t *reqpara, unsigned int *filesize)
{
    int sended, recved;
    char *reqbuf = alloca(MAX_PARA_VALUE_LEN + 16); 
    int len = pack_svmvoipB_req(reqbuf, reqpara);

    int tmpfd;
    ipport_t *addr = svmvoipB_addr;
    for (; addr != NULL; addr = addr->next) {
        if ((tmpfd = sock_connect(addr->ip, addr->port)) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "connect %s %d failed.", addr->ip, addr->port);
            continue;
        }

        if (sock_send(tmpfd, reqbuf, len, &sended) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "sent request to %s %d failed.", addr->ip, addr->port);
            close(tmpfd);
            continue;
        }

        char resbuf[12] = {0};
        char *presbuf = resbuf;
        if (sock_recv(tmpfd, resbuf, 12, &recved) <= 0) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "recv response from %s %d failed.", addr->ip, addr->port);
            close(tmpfd);
            continue;
        }
        
        unsigned short type = 0;
        unsigned short len = 0;
        unsigned int file_exist = 0;
        
        type = *(unsigned short *)(presbuf + 0);
        len = *(unsigned short *)(presbuf + 2);
        file_exist = *(unsigned int *)(presbuf + 4);
        *filesize = *(unsigned int *)(presbuf + 8);
        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "%s %d return type: %d len: %d file exist: %d, filesize: %d.", addr->ip, addr->port, type, len, file_exist, *filesize);
        
        if (type == 0x10 && len == 0x08 && file_exist == 1) {
            xapplog(LOG_INFO, APP_LOG_MASK_BASE, "%s, %u find the voice data.", addr->ip, addr->port);
            return tmpfd;
        }
        
        close(tmpfd);
    }
    
    return -1;
}

static int transfer_data_stream(int svmvoipbfd, int sockfd, unsigned int filesize)
{
    int sended;
    char sendbuf[4096] = {0};
    int len;

    //filesize += sizeof(struct wav_header);
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
    xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send %d bytes http header.", len);

#if 0
    struct wav_header header;
    bzero(&header, sizeof(header));
    wav_header_init(&header);
    /* filesize is amr file size, wav file size is not knwon, use max filesize*/
    filesize = 16 * 1024 * 5 * 60 * 60 + sizeof(struct wav_header);
    header.data_size = filesize;
    header.file_size = filesize + 36;

    len = sizeof(header);
    memcpy(sendbuf, &header, len);
    encrypt_wave_header((uint8_t *)sendbuf, len);
    if (sock_send(sockfd, sendbuf, len, &sended) == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send wav header to media error.");
        return -1;
    }
#endif
    
    unsigned int recved_len = 0;
    while (1)
    {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP)))
            break;
        
        char sendbuf[2048] = {0};
        fd_set rfds;
        struct timeval tv;
        int ret;
        int maxfd = (svmvoipbfd > sockfd) ? svmvoipbfd : sockfd;

        tv.tv_sec = 10;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(svmvoipbfd, &rfds);
        FD_SET(sockfd, &rfds);
        
        ret = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (ret == 0) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Recv voice data from mfa timeout.");
            return -1;
        }
        if (ret > 0 && FD_ISSET(svmvoipbfd, &rfds)) {
            len = recv(svmvoipbfd, sendbuf, 512, 0);
            if (len == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Failed to recv data from svmmob.");
                return -1;
            }
            if (len == 0) {
                xapplog(LOG_INFO, APP_LOG_MASK_BASE, "transfer the data successful filesize: %u, recved_len: %u", 
                        filesize, recved_len);
                return 0;
            }
            
            recved_len += len;
            ret = sock_send(sockfd, sendbuf, len, &sended);
            if (ret == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Failed to send data to media. %s", strerror(errno));
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

void *svmvoipB_thread(void *arg)
{
    int ret;
    int sockfd = (int)(unsigned long)arg;
    mfusrv_para_t *conf = mfusrv_para_get();
    ipport_t *svmvoipB_addr = conf->svmvoipB_addr;
    
    atomic_add(&g_mfu_counter.svmvoipB_req_total, 1);
    char buf[MAX_HTTP_HEADER_LEN] = {0};
    ret = recv(sockfd, buf, MAX_HTTP_HEADER_LEN - 1, 0);
    if (ret <= 0) {
        atomic_add(&g_mfu_counter.svmvoipB_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "svmvoipB recv media request failed.");
        close(sockfd);
        return NULL;
    }
    
    char parabuf[MAX_HTTP_HEADER_LEN] = {0};
    if (http_reqpara_get(parabuf, buf) == -1) {
        atomic_add(&g_mfu_counter.svmvoipB_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Failed get http request para.");
        close(sockfd);
        return NULL;
    }
    
    http_reqpara_t reqpara = {0};
    if (http_reqpara_parse(parabuf, &reqpara) == -1) {
        atomic_add(&g_mfu_counter.svmvoipB_req_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "Failed to parse http para.");
        close(sockfd);
        return NULL;
    }
    http_reqpara_show(&reqpara);    
    atomic_add(&g_mfu_counter.svmvoipB_req_valid, 1);
    
    atomic_add(&g_mfu_counter.svmvoipB_res_total, 1);
    int svmvoipbfd = 0;
    unsigned int filesize = 0;
    if ((svmvoipbfd = get_valid_svmvoipBfd(svmvoipB_addr, &reqpara, &filesize)) == -1) {
        atomic_add(&g_mfu_counter.svmvoipB_res_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to get valid svmvoipB fd.");
        close(sockfd);
        return NULL;
    }

    if ((ret = transfer_data_stream(svmvoipbfd, sockfd, filesize)) == -1) {
        //HTTP/1.1 404 Not Found
        int sended;
        if (sock_send(sockfd, HTTP_404_HEADER, sizeof(HTTP_404_HEADER) - 1, &sended) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send http 404 header to media error.");
        }
        atomic_add(&g_mfu_counter.svmvoipB_res_invalid, 1);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to transfer data stream.");
        close(sockfd);
        close(svmvoipbfd);
        return NULL;
    }

    close(svmvoipbfd);
    close(sockfd);
    atomic_add(&g_mfu_counter.svmvoipB_res_valid, 1);
    atomic_add(&g_mfu_counter.media_res_valid, 1);
    return NULL;
}
