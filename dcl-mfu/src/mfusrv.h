#ifndef __MFUSRV_H__
#define __MFUSRV_H__
#include "mfu.h"
#include <stdint.h>

#define MAX_HTTP_HEADER_LEN 4096


#define HTTP_404_HEADER     "HTTP/1.1 404 Not Found\r\n\r\n404"
#define HTTP_HEADER_CODE    "HTTP/1.1 200 OK\r\n"
#define HTTP_HEADER_SERVER  "Server: nginx/1.1.19\r\n"
#define HTTP_HEADER_DATE    "Date: Mon, 0 Dec 2015 00:00:00 GMT\r\n"
#define HTTP_HEADER_TYPE    "Content-Type: application/octet-stream\r\n"
#define HTTP_HEADER_LEN     "Content-Length: \r\n"
#define HTTP_HEADER_LAST    "Last-Modified: Mon, 22 Dec 2014 03:40:03 GMT\r\n"
#define HTTP_HEADER_CONN    "Connection: keep-alive\r\n"
#define HTTP_HEADER_ACCEPT  "Accept-Ranges: bytes\r\n"
#define HTTP_HEADER_END     "\r\n"


struct wav_header {
    uint8_t riff[4];                //4 byte , 'RIFF'
    uint32_t file_size;             //4 byte , 文件长度
    uint8_t riff_type[4];           //4 byte , 'WAVE'

    uint8_t fmt[4];                 //4 byte , 'fmt '
    uint32_t fmt_size;              //4 byte , 数值为16或18，18则最后又附加信息
    uint16_t fmt_tag;               //2 byte , 编码方式，一般为0x0001
    uint16_t fmt_channel;           //2 byte , 声道数目，1--单声道；2--双声道
    uint32_t fmt_samples_persec;    //4 byte , 采样频率
    uint32_t avg_bytes_persec;      //4 byte , 每秒所需字节数,
                                               //Byte率=采样频率*音频通道数*每次采样得到的样本位数/8
    uint16_t block_align;           //2 byte , 数据块对齐单位(每个采样需要的字节数),
                                               //块对齐=通道数*每次采样得到的样本位数/8
    uint16_t bits_persample;        //2 byte , 每个采样需要的bit数

    uint8_t data[4];                //4 byte , 'data'
    uint32_t data_size;             //4 byte ,
};




typedef enum svm_systype_e {
    SVMTELPHONE = 1,
    SVMMOBILE = 2,
    SVMVOIPA = 3,//shiwb
    SVMVOIPB = 4,//shiwb
}svm_systype_t;

typedef struct mfusrv_para_s {
    ipport_t mfuaddr;
    ipport_t *svmtel_addr;
    ipport_t *svmmob_addr;
    ipport_t *svmvoipA_addr;
    ipport_t *svmvoipB_addr;
}mfusrv_para_t;

int mfusrv_para_init(mfusrv_para_t *mfusrvpara, cmdline_para_t *cmdlpara);
void mfusrv_para_destroy(mfusrv_para_t *para);
mfusrv_para_t *mfusrv_para_get(void);

void *mfusrv_thread(void *arg);

int encrypt_wave_header(uint8_t *data, uint32_t data_len);

#define atomic_add(v, n) __sync_fetch_and_add((v), (n))

#endif
