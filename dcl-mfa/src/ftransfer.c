#include "ftransfer.h"
#include "asn.h"
#include "mfa.h"
#include "mfasrv.h"
#include "common_header.h"
#include <stdlib.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "sockop.h"
#include "amrcode.h"
#include <arpa/inet.h>
#include "g729_decoder.h"

static int readline(int fd, char *buff, int len)
{
    int i = 0;
    char c;

    for (i = 0; i < len-1; i++) {
        if (read(fd, &c, sizeof(char)) > 0) {
            buff[i] = c;
            if (c == '\n') {
                buff[i+1] = 0;
                return 0;
            }
        }else {
            break;
        }
    }

    return -1;
}

static int transfer_amr_nbsc(int fd, int sockfd)
{
    return amr2wave(fd, sockfd, AMR_CODE_NBSC);
}
static int transfer_g729_nbsc(int fd, int sockfd,unsigned int filesize)
{
    return g7292wave(fd, sockfd,filesize);
}


static int transfer_amr_nbmc(int fd, int sockfd)
{
    return amr2wave(fd, sockfd, AMR_CODE_NBMC);
}

void wav_header_init(struct wav_header *header, int tag, int bits_per_sample, int channel, int samples_per_sec)
{
    memcpy(header->riff, "RIFF", 4);
    header->file_size = 0x0;
    memcpy(header->riff_type, "WAVE", 4);

    memcpy(header->fmt, "fmt ", 4);
    header->fmt_size = 16;
    header->fmt_tag = tag;
    header->fmt_channel = channel;
    header->fmt_samples_persec = samples_per_sec;
    header->avg_bytes_persec = bits_per_sample/8 * channel * samples_per_sec;
    header->block_align = bits_per_sample/8 * channel;
    header->bits_persample = bits_per_sample;

    memcpy(header->data, "data", 4);
    header->data_size = 0x0;

    return;
}



static int send_mfu_result(int sockfd, unsigned short type,/*int systemtypes,*/ unsigned short len, unsigned int ret, unsigned int filesize)
{
    char buf[16] = {0};
    char *pbuf = buf;

    *(unsigned short *)(pbuf + 0) = type;
    *(unsigned short *)(pbuf + 2) = len;
    *(unsigned int *)(pbuf + 4) = ret;
    *(unsigned int *)(pbuf + 8) = filesize;
    //*(unsigned int *)(pbuf + 12) = systemtypes;
    
    int sended;
    if (sock_send(sockfd, buf, 12, &sended) == -1) {
        return -1;
    }

    return 0;
}

static int forecast_pcm_size_from_g729(char * path)
{
#define G729_HEADER_LEN            17        //G729文件头长度

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "open %s failed.", path);
        return -1;
    }

    int filesize = lseek(fd, 0, SEEK_END);
    int pkg_len = 0;
    int g729_sum = 0;

    lseek(fd,G729_HEADER_LEN,SEEK_SET);
    int offset = G729_HEADER_LEN;
    
    while(1)
    {
        char tmp_len = 0;
        int ret = 0;
        if((ret = read(fd, &tmp_len, 1)) == 0)
        {
            break;
        }
        else if(ret < 0)
        {
            close(fd);
            return -1;
        }

        offset += 1;

        if(tmp_len + offset > filesize)
        {
            g729_sum += pkg_len;
            break;
        }

        if(tmp_len > 0)
        {
            pkg_len = tmp_len;
        }
        
        g729_sum += pkg_len;

        lseek(fd, tmp_len, SEEK_CUR);
        offset += tmp_len;
    }

    close(fd);
    
#undef G729_HEADER_LEN
    return g729_sum * 16;
}

static int transfer_file_data(int sockfd, const char *dir, const char *filename, svmmob_req_t *req, unsigned int filesize)
{
    char path[MFA_PATH_LEN] = {0};
    char buffer[1024];
    snprintf(path, MFA_PATH_LEN, "%s/%s", dir, filename);
    int length;
    unsigned int contentlen = filesize;

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "open %s failed.", path);
        return -1;
    }

    length = strlen(filename);
    if (strncmp(filename + length - 4, ".wav", 4) == 0) {
        if (send_mfu_result(sockfd, req->type,8, 1, filesize) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send mfu result error.");
            return -1;
        }

        while ((length = read(fd, buffer, 1024)) > 0) {
            encrypt_wave_data((unsigned char *)buffer, length);
            if (write(sockfd, buffer, length) == -1) {
                close(fd);
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send data to mfu error.");
                return -1;
            }
        }
    } else if (strncmp(filename + length - 4, ".amr", 4) == 0) {
        filesize = 16/8 * 2 * 8000 * 5 * 60 * 60 + sizeof(struct wav_header);
        if (send_mfu_result(sockfd, req->type,8, 1, filesize) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send mfu result error.");
            return -1;
        }

        unsigned char sendbuf[1024] = {0};
        int len = sizeof(struct wav_header);
        struct wav_header header;
        bzero(&header, sizeof(header));
        wav_header_init(&header, 1, 16, 2, 8000);
        header.file_size = filesize;
        header.data_size = filesize - 36;
        memcpy(sendbuf, &header, len);
        encrypt_wave_data(sendbuf, len);
        send(sockfd, sendbuf, len, 0);

        char fileheader[128] = {0};
        if (readline(fd, fileheader, sizeof(fileheader)) == -1) {
            close(fd);
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "read file header failed.");
            return -1;
        }

        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "file header: %s", fileheader);
        if (!strcmp(AMRNB_SC_FILE_HEADER, fileheader)) {
            transfer_amr_nbsc(fd, sockfd);
        }else if (!strcmp(AMRNB_MC_FILE_HEADER, fileheader)) {
            int channel;
            read(fd, &channel, sizeof(int));
            transfer_amr_nbmc(fd, sockfd);
        }else if (!strcmp(AMRWB_SC_FILE_HEADER, fileheader)) {
            ;
        }else {
            close(fd);
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad file header.");
            return -1;
        }
    }
    else if(strncmp(filename + length - 5, ".g729", 5) == 0)
    {
        filesize = forecast_pcm_size_from_g729(path);
        if(filesize < 0)
        {
            filesize = 16/8 * 2 * 8000 * 5 * 60 * 60 + sizeof(struct wav_header);
        }

        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "forecast pcm size : %d", filesize);
        if (send_mfu_result(sockfd, req->type, 8, 1, filesize) == -1) {
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send mfu result error.");
            return -1;
        }

        unsigned char sendbuf[1024] = {0};
        int len = sizeof(struct wav_header);
        struct wav_header header;
        bzero(&header, sizeof(header));
        wav_header_init(&header, 1, 16, 2, 8000);
        header.file_size = filesize + sizeof(struct wav_header) - 8;
        header.data_size = filesize;
        memcpy(sendbuf, &header, len);
        encrypt_wave_data(sendbuf, len);
        send(sockfd, sendbuf, len, 0);

        char fileheader[128] = {0};
        if (readline(fd, fileheader, sizeof(fileheader)) == -1) {
            close(fd);
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "read file header failed.");
            return -1;
        }

        xapplog(LOG_INFO, APP_LOG_MASK_BASE, "file header: %s", fileheader);
        if (!strcmp(G729_FILE_HEADER, fileheader)) {
            transfer_g729_nbsc(fd,sockfd,contentlen);
        }else {
            close(fd);
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "bad file header.");
            return -1;
        }
    }
    

    close(fd);
    return 0;
}
static int search_filename(const char *dir, const char *filename)
{
    char path[MFA_PATH_LEN] = {0};

    snprintf(path, MFA_PATH_LEN, "%s/%s", dir, filename);
    struct stat stbuf = {0};
    if (stat(path,  &stbuf) == -1) {
        return -1;
    }

    if (!S_ISREG(stbuf.st_mode)) {
        return -1;
    }

    return stbuf.st_size;
}


static int unpack_svmmob_req(char *bufp, svmmob_req_t *req)
{
    unsigned short offset = 0;

    offset += 4;
    req->systypes = *(int *)(bufp + offset);
    offset += 4;
    
    offset += 4;
    req->online = *(unsigned int *)(bufp + offset);
    offset += 4;

    offset += 4;
    req->mntid = *(unsigned long long *)(bufp + offset);
    offset += 8;

    offset += 4;
    req->channel = *(unsigned int *)(bufp + offset);
    offset += 4;

    offset += 4; 
    strcpy(req->vocfile, bufp + offset);
    offset += strlen(req->vocfile) + 1;

    return (offset == req->len) ? 0 : -1;
}

static int play_history_voicesvmmobl(int sockfd, mfasrv_para_t *para, svmmob_req_t *req)
{
    char *filename = req->vocfile;
    int filesize = 0;

    char cid[16] = {0};
    char *real_file_name = strchr(filename, '/');
    if (real_file_name == NULL) {
        send_mfu_result(sockfd, req->type, 8, 0, 0);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "not found the path id by %s.", filename);
        return -1;
    }

    memcpy(cid, filename, real_file_name - filename);

    const char *path = NULL;
    int iid = atoi(cid);
    path_t *hdr = NULL;
    for (hdr = para->pathmoblhdr; hdr != NULL; hdr = hdr->next) {
        if (iid == hdr->id) {
            path = hdr->path;
        }
    }

    if (path == NULL) {
        send_mfu_result(sockfd, req->type, 8, 0, 0);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "not found the path by %s.", filename);
        return -1;
    }

    real_file_name += 1;
    if ((filesize = search_filename(path, real_file_name)) == -1) {
        send_mfu_result(sockfd, req->type, 8, 0, 0);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "not found the filename: %s/%s.", path, real_file_name);
        return -1;
    }
    if (transfer_file_data(sockfd, path, real_file_name, req, filesize) == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "transfer the filename: %s/%s error.", path, real_file_name);
        return -1;
    }
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "history playing: transfer the filename: %s/%s successful.", path, real_file_name);
    return 0;
}

static int play_history_voicesvoipA(int sockfd, mfasrv_para_t *para, svmmob_req_t *req)
{
    char *filename = req->vocfile;
    int filesize = 0;

    char cid[16] = {0};
    char *real_file_name = strchr(filename, '/');
    if (real_file_name == NULL) {
        send_mfu_result(sockfd, req->type,8, 0, 0);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "not found the path id by %s.", filename);
        return -1;
    }

    memcpy(cid, filename, real_file_name - filename);

    const char *path = NULL;
    int iid = atoi(cid);
    path_t *hdr = NULL;
    for (hdr = para->pathvoipAhdr; hdr != NULL; hdr = hdr->next) {
        if (iid == hdr->id) {
            path = hdr->path;
        }
    }

    if (path == NULL) {
        send_mfu_result(sockfd, req->type,8, 0, 0);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "not found the path by %s.", filename);
        return -1;
    }

    real_file_name += 1;
    if ((filesize = search_filename(path, real_file_name)) == -1) {
        send_mfu_result(sockfd, req->type,8, 0, 0);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "not found the filename: %s/%s.", path, real_file_name);
        return -1;
    }
    if (transfer_file_data(sockfd, path, real_file_name, req, filesize) == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "transfer the filename: %s/%s error.", path, real_file_name);
        return -1;
    }
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "history playing: transfer the filename: %s/%s successful.", path, real_file_name);
    return 0;
}

static int play_history_voicevoipB(int sockfd, mfasrv_para_t *para, svmmob_req_t *req)
{
    char *filename = req->vocfile;
    int filesize = 0;

    char cid[16] = {0};
    char *real_file_name1 = strchr(filename, '/');
    if (real_file_name1 == NULL) {
        send_mfu_result(sockfd, req->type, 8, 0, 0);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "not found the path id by %s.", filename);
        return -1;
    }

    memcpy(cid, filename, real_file_name1 - filename);

    const char *path = NULL;
    int iid = atoi(cid);
    path_t *hdr = NULL;
    for (hdr = para->pathvoipBhdr; hdr != NULL; hdr = hdr->next) {
        if (iid == hdr->id) {
            path = hdr->path;
        }
    }

    if (path == NULL) {
        send_mfu_result(sockfd, req->type, 8, 0, 0);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "not found the path by %s.", filename);
        return -1;
    }

    real_file_name1 += 1;
    if ((filesize = search_filename(path, real_file_name1)) == -1) {
        send_mfu_result(sockfd, req->type,8, 0, 0);
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "not found the filename: %s/%s.", path, real_file_name1);
        return -1;
    }
    if (transfer_file_data(sockfd, path, real_file_name1, req, filesize) == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "transfer the filename: %s/%s error.", path, real_file_name1);
        return -1;
    }
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "history playing: transfer the filename: %s/%s successful.", path, real_file_name1);
    return 0;
}


static int play_online_voice(int sockfd, svmmob_req_t *req)
{
    unsigned long long mntid = req->mntid;
    unsigned int channel = req->channel;
    online_list_t *list = online_list_get();
    online_node_t *node = online_list_find(list,req->systypes, mntid, channel);
    if (node == NULL) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "can't find the online node in online list by %llu %u.systypes=%d", mntid, channel,req->systypes);
        send_mfu_result(sockfd, req->type,8, 0, 0);
        return -1;
    }

    unsigned int filesize = 0x0FFFFFFF;
    send_mfu_result(sockfd, req->type,8, 1, filesize);

    node->sockfd = dup(sockfd);
    node->destate[0] = Decoder_Interface_init();
    node->destate[1] = Decoder_Interface_init();
    node->destate1[0] =  G729DecInit(); 
    node->destate1[1] =  G729DecInit(); 
    node->frames = 0;
    node->last_buff = NULL;
    node->last_modify_time = time(0);
    node->is_header_sended = 0;

    return 0;
}

static int get_svmmob_req(int sockfd, svmmob_req_t *req)
{
    int recved;

    if (sock_recv(sockfd, &req->type, 1, &recved) <= 0) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "recv type error.");
        return -1;
    }

    if (sock_recv(sockfd, &req->len, 2, &recved) <= 0) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "recv len error.");
        return -1;
    }

    char *buf = alloca(req->len);
    bzero(buf, req->len);

    if (sock_recv(sockfd, buf, req->len, &recved) <= 0) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "recv buf error.");
        return -1;
    }

    if (unpack_svmmob_req(buf, req) == -1) {
        atomic_add(&g_mfa_counter.req_invalid, 1); 
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "unpack svmmob request error.");
        return -1;
    }

    return 0;
}

static int deal_play_request(int sockfd)
{
    mfasrv_para_t *para = mfasrv_para_get();
    svmmob_req_t *req = alloca(sizeof(svmmob_req_t));
    bzero(req, sizeof(svmmob_req_t));

    atomic_add(&g_mfa_counter.req_total, 1); 
    if (get_svmmob_req(sockfd, req) == -1) {
        atomic_add(&g_mfa_counter.req_invalid, 1); 
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to get svmmob request.");
        return -1;
    }
    atomic_add(&g_mfa_counter.req_valid, 1); 
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "type: %u, len: %u, online: %u, mntid: %u, channel: %u, vocfile: %s, systypes:%u", req->type, req->len, req->online, req->mntid, req->channel, req->vocfile,req->systypes);

    atomic_add(&g_mfa_counter.res_total, 1); 
    if (req->online == 0x00) { //play history voice
        if(req->systypes == 2)//svmmobl
        {
            if (play_history_voicesvmmobl(sockfd, para, req) == -1) {
                atomic_add(&g_mfa_counter.res_invalid, 1);      
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to play history voice.");
                return -1;
            }
        }
        if(req->systypes == 3)//voipA
        {
            if (play_history_voicesvoipA(sockfd, para, req) == -1) {
                atomic_add(&g_mfa_counter.res_invalid, 1);      
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to play history voice.");
                return -1;
            }
        }
        if(req->systypes == 4)//voipB
        {
            if (play_history_voicevoipB(sockfd, para, req) == -1) {
                atomic_add(&g_mfa_counter.res_invalid, 1);      
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to play history voice.");
                return -1;
            }
        }
        
    }
    else if (req->online == 0x01){ //play online voice
        if (play_online_voice(sockfd, req) == -1) {
            atomic_add(&g_mfa_counter.res_invalid, 1);      
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "failed to play online voice.");
            return -1;
        }
    }else {
        atomic_add(&g_mfa_counter.res_invalid, 1);      
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "can't recongnize the online: %u.", req->online);
        return -1;
    }
    atomic_add(&g_mfa_counter.res_valid, 1); 

    return 0;
}


static void asncallback(unsigned int tag, int constructed, unsigned char * content, int contentlen, unsigned int *layer, int curlayer, void * para)
{
    int i;
    realtime_voice_t *voice = (realtime_voice_t *)para;

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
            sscanf(buf, "%llu", &voice->mntid);
            voice->mntid -= 200;
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
            voice->channel = *(unsigned int *)buf;
        }
    }
    //3. sequence
    if (curlayer == 3 && tag == 0x84) {
        unsigned char cmplayer[] = {0x30, 0xA1, 0x84};
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
            voice->vocsequence = *(unsigned long *)buf;
        }
    }
    //4. voctime
    if (curlayer == 3 && tag == 0x85) {
        unsigned char cmplayer[] = {0x30, 0xA1, 0x85};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res && contentlen == 0x0E) {
            memcpy(voice->voctime, content, 0x0E);
        }
    }
    //5. vocdirect
    if (curlayer == 9 && tag == 0x80) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA1, 0x30, 0xA2, 0xAB, 0xA3, 0xA0, 0x80};
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
            voice->vocdirect = *(unsigned long *)buf;
        }
    }
    //6. vocencode
    if (curlayer == 8 && tag == 0x81) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA1, 0x30, 0xA2, 0xAB, 0xA3, 0x81};
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
            voice->vocencode = *(unsigned long *)buf;
        }
    }
    //7. vocframe
    if (curlayer == 8 && tag == 0x87) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA1, 0x30, 0xA2, 0xAB, 0xA3, 0x87};
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
            voice->vocframe = *(unsigned long *)buf;
        }
    }
    //8. vocdata voclen
    if (curlayer == 7 && tag == 0x81) {
        unsigned char cmplayer[] = {0x30, 0xA2, 0xA1, 0x30, 0xA2, 0xAB, 0x81};
        bool res = true;
        for (i = 0; i < curlayer; i++) {
            if (layer[i] != cmplayer[i]) {
                res = false;
            }
        }
        if (res) {
            voice->voclen = contentlen;
            voice->vocdata = calloc(1, contentlen);
            memcpy(voice->vocdata, content, contentlen);
        }
    }

    return ;
}

static int parse_realtime_voice(unsigned char *buf, unsigned int nbuf, realtime_voice_t *voice)
{
    unsigned char * pkt = buf;
    int len = nbuf;

    int os = 0;
    while(len > 0)
    {
        unsigned int layer[32];
        int asnlen = deasn(pkt+os, len, voice, layer, 1, 32, asncallback);
        os += asnlen;
        len -= asnlen;
    }

    return 0;
}

static int get_realtime_voice(int sockfd, realtime_voice_t *voice,int systypes)
{    
    unsigned char buf1[2] = {0};
    unsigned char type = 0;
    unsigned char dlen = 0;
    int i;
    recv(sockfd, buf1, 2, 0);
    type = buf1[0];
    dlen = buf1[1];

    unsigned int len;
    unsigned char *buf2 = NULL;
    unsigned int lenbytes = 0;
    if (dlen & 0x80) {
        lenbytes = dlen & 0x7F;    
        buf2 = alloca(lenbytes);
        recv(sockfd, buf2, lenbytes, 0);
        unsigned char buf[16] = {0};
        for (i = 0; i < lenbytes; i++) {
            buf[i+1] = buf2[lenbytes - i - 1];
        }
        len = *(unsigned int *)(buf+1);
    }else {
        len = dlen;
    }
    //xapplog(LOG_INFO, APP_LOG_MASK_BASE, "len = %d\n", len);

    int recved;
    unsigned char *buf3 = alloca(len);
    if (sock_recv(sockfd, buf3, len, &recved) == -1) {
        return -1;
    }

    int nbuf = 2 + lenbytes + len;
    unsigned char *buf = alloca(nbuf);
    memcpy(buf, buf1, 2);
    if (lenbytes > 0) {
        memcpy(buf + 2, buf2, lenbytes);
    }
    memcpy(buf + 2 + lenbytes, buf3, len);
    voice ->systypes = systypes;
    if (parse_realtime_voice(buf, nbuf, voice) == -1) {
        return -1;
    }

    
    if (voice->vocframe != VOC_DIRECT_NONE) {
        return -1;
    }

    if (voice->vocframe != VOC_FRAME_VOC) {
        return -1;
    }

    return 0;
}

static int send_realtime_voice(online_node_t *node, realtime_voice_t *voice)
{
    if (node == NULL || node->sockfd == -1) {
        return 0;
    }

    int sended;

    if (voice->vocencode == VOC_ENCODE_PCM) {
        if (!node->is_header_sended) {
            unsigned int filesize = 0x0FFFFFFF;
            unsigned char sendbuf[1024] = {0};
            int len = sizeof(struct wav_header);
            struct wav_header header;
            int tag = 6;
            int bits_per_sample = 8;
            int channel = 2;
            int samples_per_sec = 8000;
            bzero(&header, sizeof(header));
            wav_header_init(&header, tag, bits_per_sample, channel, samples_per_sec);
            header.file_size = filesize;
            header.data_size = filesize - 36;
            memcpy(sendbuf, &header, len);
            encrypt_wave_data(sendbuf, len);
            send(node->sockfd, sendbuf, len, 0);
            node->is_header_sended = 1;
        }

        encrypt_wave_data(voice->vocdata, voice->voclen);
        if (sock_send(node->sockfd, voice->vocdata, voice->voclen, &sended) == -1) {
            return -1;
        }
    }else if (voice->vocencode == VOC_ENCODE_AMR) {
        if (!node->is_header_sended) {
            unsigned int filesize = 0x0FFFFFFF;
            unsigned char sendbuf[1024] = {0};
            int len = sizeof(struct wav_header);
            struct wav_header header;
            int tag = 1;
            int bits_per_sample = 16;
            int channel = 2;
            int samples_per_sec = 8000;
            bzero(&header, sizeof(header));
            wav_header_init(&header, tag, bits_per_sample, channel, samples_per_sec);
            header.file_size = filesize;
            header.data_size = filesize - 36;
            memcpy(sendbuf, &header, len);
            encrypt_wave_data(sendbuf, len);
            send(node->sockfd, sendbuf, len, 0);
            node->is_header_sended = 1;
        }

        if (node->frames % 2 == 0) {
            if (node->last_buff) free(node->last_buff);
            node->last_buff = calloc(1, voice->voclen);
            memcpy(node->last_buff, voice->vocdata, voice->voclen);
        }else {
            short wavbuff0[160];
            short wavbuff1[160];
            short wavbuff[320];

            Decoder_Interface_Decode(node->destate[0], node->last_buff, wavbuff0, 0);
            Decoder_Interface_Decode(node->destate[1], voice->vocdata, wavbuff1, 0);

            int i = 0;
            for (; i < 160; i++) {
                wavbuff[2*i] = wavbuff0[i];
                wavbuff[2*i+1] = wavbuff1[i];
            }

            encrypt_wave_data((uint8_t *)wavbuff, 320*sizeof(short));
            if (sock_send(node->sockfd, wavbuff, 320*sizeof(short), &sended) == -1) {
                return -1;
            }
        }
    }else if (voice->vocencode == VOC_ENCODE_PCM_U) {
        if (!node->is_header_sended) {
            unsigned int filesize = 0x0FFFFFFF;
            unsigned char sendbuf[1024] = {0};
            int len = sizeof(struct wav_header);
            struct wav_header header;                
            int tag = 7;
            int bits_per_sample = 8;
            int channel = 2;
            int samples_per_sec = 8000;
            bzero(&header, sizeof(header));
            wav_header_init(&header, tag, bits_per_sample, channel, samples_per_sec);
            header.file_size = filesize;
            header.data_size = filesize - 36;
            memcpy(sendbuf, &header, len);
            encrypt_wave_data(sendbuf, len);
            send(node->sockfd, sendbuf, len, 0);
            node->is_header_sended = 1;
        }

        encrypt_wave_data(voice->vocdata, voice->voclen);
        if (sock_send(node->sockfd, voice->vocdata, voice->voclen, &sended) == -1) {
            return -1;
        }
    }
    else if(voice->vocencode == VOC_ENCODE_G729)
    {
        if (!node->is_header_sended) {
            unsigned int filesize = 0x0FFFFFFF;
            unsigned char sendbuf[1024] = {0};
            int len = sizeof(struct wav_header);
            struct wav_header header;
            int tag = 1;
            int bits_per_sample = 16;
            int channel = 2;
            int samples_per_sec = 8000;
            bzero(&header, sizeof(header));
            wav_header_init(&header, tag, bits_per_sample, channel, samples_per_sec);
            header.file_size = filesize;
            header.data_size = filesize - 36;
            memcpy(sendbuf, &header, len);
            encrypt_wave_data(sendbuf, len);
            send(node->sockfd, sendbuf, len, 0);
            node->is_header_sended = 1;
        }
            unsigned char buf_up[20];
            unsigned char buf_down[20];
            short buf_mix[320];
            short *pcm_up;
            short *pcm_down;
            int i;
        
            int offset = 0;
            int n = 0;
                    
            n +=1;    
            memcpy(buf_up, voice->vocdata + n + offset, 20); 
            pcm_up = G729Decode(node->destate1[0], buf_up);
            offset += 20;    
            n +=1;    
            memcpy(buf_down, voice->vocdata + n + offset, 20);    
            pcm_down = G729Decode(node->destate1[1], buf_down);    
            for (i = 0; i < 160; i++)    
            {                    
                buf_mix[i * 2] = pcm_up[i];      
                buf_mix[i * 2 + 1] = pcm_down[i];  
            }          
            encrypt_wave_data((uint8_t *)buf_mix, 320*sizeof(short));
            if (sock_send(node->sockfd, buf_mix, 320*sizeof(short), &sended) == -1) {
                return -1;
            }
    }

    node->frames++;
    node->last_modify_time = time(0);

    return 0;
}

static int online_list_append_voice(online_list_t *list, realtime_voice_t *voice)
{
    online_node_t *node = calloc(1, sizeof(online_node_t));
    node->sockfd = -1;
    node->mntid = voice->mntid;
    node->channel = voice->channel;
    node->systypes = voice->systypes;//liqian
    node->last_modify_time = time(0);
    online_list_append(list, node);

    return 0;
}


static int deal_realtime_voice(int sockfd,int systypes)
{
    int done = 0;
    int mfasrv_reload_time = g_mfasrv_reload_time;
    realtime_voice_t voice;
    while (!done) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }

        if (unlikely(mfasrv_reload_time != g_mfasrv_reload_time)) {
            break;
        }

        if (online_list_timeout_check(online_list_get()) == -1) {
            break;
        }
        
        if (sock_readable(sockfd, 3, 0)) {
            bzero(&voice, sizeof(realtime_voice_t));
            if (get_realtime_voice(sockfd, &voice,systypes) == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "get realtime voice error, closethelink with svm.");
                if (voice.vocdata) free(voice.vocdata);
                return -1;
            }

            online_list_t *list = online_list_get();
            online_node_t *node = NULL;
            if ((node = online_list_find(list, voice.systypes,voice.mntid, voice.channel)) == NULL) {
                online_list_append_voice(list, &voice);
                xapplog(LOG_INFO, APP_LOG_MASK_BASE, "store a new realtime voice node. mntid: %d, channel: %d", voice.mntid, voice.channel,voice.channel);
            }
        //    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "store a new realtime voice node.systypes:%d mntid: %d, channel: %d",
        //    voice.systypes,voice.mntid );
            if (send_realtime_voice(node, &voice) == -1) {
                 xapplog(LOG_ERR, APP_LOG_MASK_BASE, "send realtime voice error, remove the node in list.");
                if (node->sockfd != -1) {
                     close(node->sockfd);
                    node->sockfd = -1;
                    Decoder_Interface_exit(node->destate[0]);
                    Decoder_Interface_exit(node->destate[1]);
                    G729DecClose(node->destate1[0]); 
                    G729DecClose(node->destate1[1]);
                    if (node->last_buff != NULL) {
                        free(node->last_buff);
                        node->last_buff = NULL;
                    }
                    node->is_header_sended = 0;
                }
            }

            free(voice.vocdata);
        }
    }

    return 0;
}

void *ftransfer_thread(void *arg)
{
    //int sockfd = (int)(unsigned long)arg;
    ipportsockfd *ipport = (ipportsockfd *)arg;
    unsigned char systype = CLIENT_SYSTEM_SVM;

    if (sock_readable(ipport->socketfd, 10, 0)) {
        if (recv(ipport->socketfd, &systype, 1, MSG_PEEK) <= 0) {
            close(ipport->socketfd);
            xapplog(LOG_ERR, APP_LOG_MASK_BASE, "recv(PEEK) error.");
            return NULL;
        }
    }
    if (systype == CLIENT_SYSTEM_MFU) { /* data from mfu */
            if (deal_play_request(ipport->socketfd) == -1) {
                close(ipport->socketfd);
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "deal play request error.");
                return NULL;
            }
        }
    else
    {
        switch(ipport->systypes)
        {
            case 2://svmmobl
                xapplog(LOG_INFO, APP_LOG_MASK_BASE, "detected realtime svmmobl voice.");
                if (deal_realtime_voice(ipport->socketfd,ipport->systypes) == -1) {
                     close(ipport->socketfd);
                    xapplog(LOG_ERR, APP_LOG_MASK_BASE, "deal realtime svmmobl voice error.");
                    return NULL;
                    }
                break;
            case 3://voipA
                
                xapplog(LOG_INFO, APP_LOG_MASK_BASE, "detected realtime voice voipA.");
                if (deal_realtime_voice(ipport->socketfd,ipport->systypes) == -1) {
                     close(ipport->socketfd);
                    xapplog(LOG_ERR, APP_LOG_MASK_BASE, "deal realtime voice error.");
                    return NULL;
                }
                break;
            case 4:  //voipB
                xapplog(LOG_INFO, APP_LOG_MASK_BASE, "detected realtime voipB voice.");
                if (deal_realtime_voice(ipport->socketfd,ipport->systypes) == -1) {
                     close(ipport->socketfd);
                    xapplog(LOG_ERR, APP_LOG_MASK_BASE, "deal realtime voice voipB error.");
                    return NULL;
                }
                break;
            default:
                break;
        }
    }
    close(ipport->socketfd);
    return NULL;
}
