#include "amrcode.h"
#include "mfa.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <stdint.h>
#include "g729_decoder.h"
#include "ftransfer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "../../common/applog.h"
#include "../../common/lt_list.h"

typedef struct
{
    unsigned char *bits;
    hlist_node_t node;
}st_g729_segment_t;

pthread_mutex_t g_g729_decoder_mutex;

#define G729_HEADER_LEN            17        //G729文件头长度
#define DECODE_LEN              10

#define GRACEFUL_FREE(p) \
({\
    if( p!=NULL )\
    {\
        free (p);\
        p = NULL;\
    }\
 })

#define GRACEFUL_INSERT_LIST(list, type) \
({\
    type * seg = (type *)malloc(sizeof(type));\
    if(NULL == seg)\
    {\
        xapplog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "ERROR: fail to malloc()", __FILE__, __LINE__);\
        return -1;\
    }\
    hlist_insert_tail(list, &seg->node);\
    seg;\
})

#define DESTORY_SEG_LIST(list) \
({\
    if(list)\
    {\
        while(list->node_cnt > 0)\
        {\
            hlist_node_t * node = hlist_get_head_node(list);\
            st_g729_segment_t *p = container_of(node, st_g729_segment_t, node);\
            free(p);\
        }\
    }\
    GRACEFUL_FREE(list);\
})

amrnb_code_t amrnb_code[MAX_AMRNB_CODE] = {
    {0, "AMR475",    4.75*1000,    13, 0x04},
    {1, "AMR515",    5.15*1000,    14, 0x0c},
    {2, "AMR590",    5.90*1000,    16, 0x14},
    {3, "AMR670",    6.70*1000,    18, 0x1c},
    {4, "AMR740",    7.40*1000,    20, 0x24},
    {5, "AMR795",    7.95*1000,    21, 0x2c},
    {6, "AMR1020",    10.20*1000, 27, 0x34},
    {7, "AMR1220",    12.20*1000, 32, 0x3c},
};

int bps2framesize(double bps)
{
    return (int)((bps/50/8) + 0.5) + 1;
}

int amrnb_code_index(int frameheader)
{
    int i = 0;
    
    for (i = 0; i < MAX_AMRNB_CODE; i++) {
        if (amrnb_code[i].frame_header == frameheader) {
            return i;
        }
    }

    return -1;
}


static int write_buff(int fd, void *buff, int len)
{
    int    n = 0;
    int ret = 0;

    while (n < len) {
        ret = write(fd, buff + n, len - n);
        if (ret == -1) {
            return -1;
        }
        n += ret;
    }

    return 0;
}

static int amrnbsc2wave(int amrfd, int wavefd)
{
    unsigned char amrbuff[1024];
    short wavebuff[256];
    short buff[1024];
    unsigned char frameheader = 0;
    int index = 0;
    int frame_data_size = 0;
    int *destate = NULL;
    int datasize = 0;
    int wavelen = 160;
    int i = 0;
    /*if amrfd is a file, skip the file header*/
    if (read(amrfd, &frameheader, 1) != 1) {
        return -1;
    }

    if (frameheader == '#') {
        int len = sizeof(AMRNB_SC_FILE_HEADER) - 1 - 1;
        if (read(amrfd, amrbuff, len) != len) {
            return -1;
        }
        if (read(amrfd, &frameheader, 1) != 1) {
            return -1;
        }
    }

    destate = Decoder_Interface_init();
    while (1) {
        index = amrnb_code_index(frameheader);
        if (index == -1) {
            amrbuff[0] = frameheader;
            memset(amrbuff+1, 0, sizeof(amrbuff)-1);
        }else {
            amrbuff[0] = frameheader;
            frame_data_size = amrnb_code[index].frame_size - 1;
            if (read(amrfd, amrbuff + 1, frame_data_size) == 0) {
                datasize = -1;
                break;
            }
        }
        
        Decoder_Interface_Decode(destate, amrbuff, wavebuff, 0);
        
        for (i = 0; i < wavelen; i++) {
            buff[i*2] = wavebuff[i];
            buff[i*2 + 1] = wavebuff[i];
        }
        datasize += 2*wavelen*sizeof(short);
        encrypt_wave_data((unsigned char *)buff, datasize);
        if (write_buff(wavefd, buff, datasize) == -1) {
            datasize = -1;
            break;
        }

        if (read(amrfd, &frameheader, 1) != 1) {
            datasize = -1;
            break;
        }
    }

    Decoder_Interface_exit(destate);

    return datasize;
}

static int amrnbmc2wave(int amrfd, int wavefd)
{
    /*if amrfd is a file, skip the file header*/
    unsigned char amrbuff[1024];
    short wavebuff[2][256];
    short buff[1024];
    int frame_index = 0;
    unsigned char frameheader = 0;
    int index = 0;
    int frame_data_size = 0;
    int data_size = 0;
    int *destate[2] = {NULL, NULL};
    int nwave = 160;
    int i = 0;

    /*if amrfd is a file, skip the file header*/
    if (read(amrfd, &frameheader, 1) != 1) {
        return -1;
    }

    if (frameheader == '#') {
        int len = sizeof(AMRNB_MC_FILE_HEADER) - 1 - 1 + 4;
        if (read(amrfd, amrbuff, len) != len) {
            return -1;
        }
        if (read(amrfd, &frameheader, 1) != 1) {
            return -1;
        }
    }

    destate[0] = Decoder_Interface_init();
    destate[1] = Decoder_Interface_init();
    for (; 1; frame_index++) {
        index = amrnb_code_index(frameheader);
        if (index == -1) {
            amrbuff[0] = frameheader;
            memset(amrbuff+1, 0, sizeof(amrbuff)-1);
        }else {
            amrbuff[0] = frameheader;
            frame_data_size = amrnb_code[index].frame_size - 1;
            if (read(amrfd, amrbuff + 1, frame_data_size) == 0) {
                break;
            }
        }
        
        Decoder_Interface_Decode(destate[frame_index % 2], amrbuff, wavebuff[frame_index % 2], 0);
        
        if ((frame_index % 2) == 1) {
            for (i = 0; i < nwave; i++) {
                buff[2*i] = wavebuff[0][i];    
                buff[2*i+1] = wavebuff[1][i];    
            }

            encrypt_wave_data((unsigned char *)buff, nwave*sizeof(short)*2);
            if (write_buff(wavefd, buff, nwave*sizeof(short)*2) == -1) {
                data_size = -1;    
                break;
            }

            data_size += nwave*sizeof(short)*2;
        }

        if (read(amrfd, &frameheader, 1) != 1) {
            break;
        }
    }

    Decoder_Interface_exit(destate[0]);
    Decoder_Interface_exit(destate[1]);

    return data_size;
}

static int amrwbsc2wave(int amrfd, int wavefd)
{
    return 0;
}
static int amrwbmc2wave(int amrfd, int wavefd)
{
    return 0;
}

int amr2wave(int amrfd, int wavefd, int amrcode)
{
    switch (amrcode) {
        case AMR_CODE_NBSC:
            return amrnbsc2wave(amrfd, wavefd);
            break;
        case AMR_CODE_NBMC:
            return amrnbmc2wave(amrfd, wavefd);
            break;
        case AMR_CODE_WBSC:
            return amrwbsc2wave(amrfd, wavefd);
            break;
        case AMR_CODE_WBMC:
            return amrwbmc2wave(amrfd, wavefd);
            break;
        default:
            return -1;
    }

    return 0;
}

void wav_header_init(struct wav_header *header, int tag, int bits_per_sample, int channel, int samples_per_sec);

static int decode_g729_channel(hlist_head_t *list, short *pcm_buf, int pcm_len)
{
    if(NULL == list || NULL == pcm_buf)
    {
        return -1;
    }
    
    G729DECODER *decoder;                              //解码器
    int pcm_offset = 0;

    pthread_mutex_lock(&g_g729_decoder_mutex);
    decoder = G729DecInit();

    hlist_node_t *pnode = list->first;
    while(pnode)
    {
        if(pcm_offset + PCM_BUF_LEN * sizeof(short) > pcm_len) break;
        
        st_g729_segment_t * seg = container_of(pnode, st_g729_segment_t, node);
        if(seg->bits)
        {
            short * pcm = G729Decode(decoder, seg->bits);
            memcpy((unsigned char*)pcm_buf + pcm_offset, pcm, PCM_BUF_LEN * sizeof(short));
        }
        else
        {
            memset((unsigned char*)pcm_buf + pcm_offset, 0x00, PCM_BUF_LEN * sizeof(short));
        }
        
        pcm_offset += PCM_BUF_LEN * sizeof(short);
        pnode = pnode->next;
    }

    G729DecClose(decoder); 
    pthread_mutex_unlock(&g_g729_decoder_mutex);
    return 0;
}


static void debug_save_channel_to_wav(short *pcm, int len, char *path)
{
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "[DEBUG save wav file] start save wav file, file %s", path);
    struct wav_header header;
    int save_wav_fd = open(path, O_TRUNC | O_CREAT | O_WRONLY);
    extern int errno;
    
    if(save_wav_fd < 0)
    {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "[DEBUG save wav file]fail to open %s,ERROR : %s", "./test.wav", strerror(errno));
        return;
    }

    bzero(&header, sizeof(struct wav_header));
    wav_header_init(&header, 1, 16, 1, 8000);
    
    header.file_size = len + sizeof(struct wav_header) - 8;
    header.data_size = len;

    write(save_wav_fd, &header, sizeof(struct wav_header));
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "[DEBUG save wav file] start save wav file, file %s", path);
    write(save_wav_fd, pcm, len);
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "[DEBUG save wav file] start save wav file done, file %s", path);
    close(save_wav_fd);
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "[DEBUG save wav file] file closed");
}

static int fill_up_down_seg_list(unsigned char *buf, int size, hlist_head_t *up_list, hlist_head_t *down_list)
{
    unsigned char seg_len = 0;
    unsigned char cur_seg_len = 0;
    int offset = G729_HEADER_LEN;

    while(1)
    {
        if(offset >= size) break;
        // 读上行包长度
        cur_seg_len = buf[offset];
        if(seg_len ==0 && cur_seg_len != 0) seg_len = cur_seg_len;
        offset ++;
        
        if(offset + cur_seg_len > size) break;
        //上行包的指针填入链表
        if(seg_len != 0)
        {
            int i = 0;
            for(i = 0; i < seg_len / DECODE_LEN; i++)
            {
                st_g729_segment_t * seg = GRACEFUL_INSERT_LIST(up_list, st_g729_segment_t);
                if(cur_seg_len == 0)
                {
                    seg->bits = NULL;
                }
                else
                {
                    seg->bits = &buf[offset];
                    offset += DECODE_LEN;
                }
            }
        }

        if(offset >= size) break;
        // 读下行包长度
        cur_seg_len = buf[offset];
        if(seg_len ==0 && cur_seg_len != 0) seg_len = cur_seg_len;
        offset ++;

        if(offset + cur_seg_len > size) break;
        //下行包指针填入链表
        if(seg_len != 0)
        {
            int i = 0;
            for(i = 0; i < seg_len / DECODE_LEN; i++)
            {
                st_g729_segment_t * seg = GRACEFUL_INSERT_LIST(down_list, st_g729_segment_t);
                if(cur_seg_len == 0)
                {
                    seg->bits = NULL;
                }
                else
                {
                    seg->bits = &buf[offset];
                    offset += DECODE_LEN;
                }
            }
        }
    }
    
    return 0;
}

/* biref: 读YJ的.g729语音文件，将g729编码的语音解码为pcm，并通过socket发送出去
 * param: @ int g729fd .g729格式的历史语音的文件描述
 *         @ int wavefd socket描述符
 *         @ unsigned int filesize .g729语音文件出去头部之后的大小
 * history: 
 *       1. [李涛] 修改原因: (1)当前版本不能处理40字节包长; (2)没有对空音包添加静音
 *       2. [李涛] 修改原因: g729解码库线程不安全，上下行同时解码会有一边是噪声，将上下行先后解码
 *       3. [李涛] 修改原因：g729线程不安全，但是却没有加锁；先后解码的时候没有做好填充静音帧
 */
int g7292wave(int g729fd, int wavefd,unsigned int filesize)
{
    int ret = 0;
    int datalen = 0;                    //最终发出的pcm长度
    unsigned char *filebuf = NULL;      //缓存整个g729文件
    hlist_head_t * up_seg_list = NULL; //上行包链表
    hlist_head_t * down_seg_list = NULL;    //下行包链表
    short * pcm_up = NULL;              //上行pcm
    short * pcm_down = NULL;            //下行pcm
    short *mixer = NULL;                //混合器
    int pcm_len = 0;                    //上行或下行pcm的长度，单位为字节

    filebuf = (unsigned char *)malloc(filesize);
    up_seg_list = hlist_create();
    down_seg_list = hlist_create();
    if((NULL == filebuf) || (NULL == up_seg_list) || (NULL == down_seg_list))
    {
        xapplog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "ERROR: fail to malloc()");
        datalen = -1;
        goto G7292WAVE_ERROR_MALLOC;
    }

    lseek(g729fd, 0, SEEK_SET);
    ret = read(g729fd, filebuf, filesize);
    if(ret != filesize)
    {
        datalen = -1;
        xapplog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "reading g729 file failed, ret: %d, filesize: %d", ret, filesize);
        goto G7292WAVE_ERROR_READ;
    }
    xapplog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "reading g729 file sucess, filesize: %d", filesize);

    if(fill_up_down_seg_list(filebuf, filesize, up_seg_list, down_seg_list) < 0)
    {
        xapplog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "ERROR: fail to fill_up_down_seg_list()");
        datalen = -1;
        goto G7292WAVE_ERROR_FILL_LIST;
    }
    xapplog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "fill_up_down_seg_list() sucess, up seg cnt: %d, down seg cnt: %d",
            up_seg_list->node_cnt, down_seg_list->node_cnt);

    //包的个数，每个包10字节
    pcm_len = (up_seg_list->node_cnt > down_seg_list->node_cnt) ? up_seg_list->node_cnt : down_seg_list->node_cnt;
    //每10个字节的g729字节流，会被解码为80个short
    pcm_len *= PCM_BUF_LEN * sizeof(short);
    xapplog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "pcm_len: %d", pcm_len);
    
    //分配空间
    pcm_up = (short *)malloc(pcm_len);      //上行pcm
    pcm_down = (short *)malloc(pcm_len);    //下行pcm
    mixer = (short *)malloc(pcm_len * 2);   //混合器
    if((NULL == pcm_up) || (NULL == pcm_down) || (NULL == mixer))
    {
        xapplog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "[%s:%d] ERROR: fail to malloc()", __FILE__, __LINE__);
        datalen = -1;
        goto G7292WAVE_ERROR_MALLOC_2;
    }

    //解码
    if(decode_g729_channel(up_seg_list, pcm_up, pcm_len) < 0 ||
        decode_g729_channel(down_seg_list, pcm_down, pcm_len) < 0)
    {
        goto G7292WAVE_ERROR_MALLOC_2;
        datalen = -1;
    }

    //调试，将上下行分别存wav文件
#define DEBUG_G729_SAVE_FILE    0
#if DEBUG_G729_SAVE_FILE
    debug_save_channel_to_wav(pcm_up, pcm_len, "/tmp/debug_up.wav");
    debug_save_channel_to_wav(pcm_down, pcm_len, "/tmp/debug_down.wav");
#endif
#undef DEBUG_G729_SAVE_FILE

    //混合
    int i = 0;
    for(i = 0; i < pcm_len / sizeof(short); i++)
    {
        mixer[2 * i] = pcm_up[i];
        mixer[2 * i + 1] = pcm_down[i];
    }

    //对混合器加密
    encrypt_wave_data((unsigned char *)mixer, pcm_len * 2);
    
    //写socket
    datalen = write(wavefd, mixer, pcm_len * 2);

G7292WAVE_ERROR_MALLOC_2:
    GRACEFUL_FREE(pcm_up);
    GRACEFUL_FREE(pcm_down);
    GRACEFUL_FREE(mixer);

G7292WAVE_ERROR_FILL_LIST:
    DESTORY_SEG_LIST(up_seg_list);
    DESTORY_SEG_LIST(down_seg_list);

G7292WAVE_ERROR_READ:
    GRACEFUL_FREE(filebuf);
    GRACEFUL_FREE(up_seg_list);
    GRACEFUL_FREE(down_seg_list);

G7292WAVE_ERROR_MALLOC:
    close(g729fd);
    close(wavefd);
    
    return datalen;
}
