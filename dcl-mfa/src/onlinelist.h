#ifndef __ONLINELIST_H__
#define __ONLINELIST_H__
#include <pthread.h>
#include <stdbool.h>
#include "g729_decoder.h"

#define VOC_DIRECT_UP    0x01
#define VOC_DIRECT_DOWN  0x02
#define VOC_DIRECT_NONE  0x03

#define VOC_ENCODE_EFR   0x6E
#define VOC_ENCODE_AMR   0x70
#define VOC_ENCODE_PCM   0x08
#define VOC_ENCODE_PCM_U    0x00
#define VOC_ENCODE_G729  0x12

#define VOC_FRAME_IP     0x00
#define VOC_FRAME_UDP    0x01
#define VOC_FRAME_RTP    0x02
#define VOC_FRAME_VOC    0x03

typedef struct realtime_voice_s {
    int systypes;
    unsigned long long mntid;   //MNTID
    unsigned int channel; //通信识别码
    unsigned int vocsequence; //序号
    unsigned char voctime[20];   //时间戳, 字符串形式YYYYMMDDHHMMSS
    unsigned char vocdirect; //1-up, 2-down, 3-不区分
    unsigned char vocencode; //0x00-PCM U率; 0x08-PCM A率; 0x6E-EFR; 0x6F-AMR-WB; 0x70-AMR
    unsigned char vocframe; //帧类型，指上面的“语音数据”的帧格式，0-IP帧；1-UDP帧；2-RTP帧；3-语音帧。一般都是3，即纯语音数据
    unsigned int voclen;
    unsigned char *vocdata;
}realtime_voice_t; 

void realtime_voice_free(realtime_voice_t *voice);
realtime_voice_t *realtime_voice_calloc(void);
realtime_voice_t *realtime_voice_dup(realtime_voice_t *voice);

typedef struct online_node_s {
    int sockfd;
    int systypes;
    unsigned int mntid;
    unsigned int channel;
    unsigned char *last_buff;
    int *destate[2];
    G729DECODER *destate1[2];
    int frames;
    time_t last_modify_time;
    int is_header_sended;
    
    struct online_node_s *next;
    struct online_node_s *prev;
}online_node_t;
void online_node_free(online_node_t *node);

typedef struct online_list_s {
    online_node_t *header;
    pthread_mutex_t mutex;
}online_list_t;

void online_list_init(online_list_t *list);
bool online_list_is_empty(online_list_t *list);
void online_list_append(online_list_t *list, online_node_t *node);
void online_list_remove(online_list_t *list, online_node_t *node);
int  online_list_timeout_check(online_list_t *list);
online_node_t *online_list_find(online_list_t *list, int systemtypes,unsigned long long mntid, unsigned int channel);
void online_list_clear(online_list_t *list);
void online_list_destroy(online_list_t *list);

#endif
