#ifndef __FTRANSFER_H__
#define __FTRANSFER_H__
#include <stdint.h>

#define atomic_add(v, n) __sync_fetch_and_add((v), (n))
void *ftransfer_thread(void *arg);

#define CLIENT_SYSTEM_MFU        0x10
#define CLIENT_SYSTEM_SVM        0x30

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




#include "onlinelist.h"
#endif
