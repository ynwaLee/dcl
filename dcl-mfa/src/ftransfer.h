#ifndef __FTRANSFER_H__
#define __FTRANSFER_H__
#include <stdint.h>

#define atomic_add(v, n) __sync_fetch_and_add((v), (n))
void *ftransfer_thread(void *arg);

#define CLIENT_SYSTEM_MFU        0x10
#define CLIENT_SYSTEM_SVM        0x30

struct wav_header {
    uint8_t riff[4];                //4 byte , 'RIFF'
    uint32_t file_size;             //4 byte , �ļ�����
    uint8_t riff_type[4];           //4 byte , 'WAVE'

    uint8_t fmt[4];                 //4 byte , 'fmt '
    uint32_t fmt_size;              //4 byte , ��ֵΪ16��18��18������ָ�����Ϣ
    uint16_t fmt_tag;               //2 byte , ���뷽ʽ��һ��Ϊ0x0001
    uint16_t fmt_channel;           //2 byte , ������Ŀ��1--��������2--˫����
    uint32_t fmt_samples_persec;    //4 byte , ����Ƶ��
    uint32_t avg_bytes_persec;      //4 byte , ÿ�������ֽ���,
                                               //Byte��=����Ƶ��*��Ƶͨ����*ÿ�β����õ�������λ��/8
    uint16_t block_align;           //2 byte , ���ݿ���뵥λ(ÿ��������Ҫ���ֽ���),
                                               //�����=ͨ����*ÿ�β����õ�������λ��/8
    uint16_t bits_persample;        //2 byte , ÿ��������Ҫ��bit��

    uint8_t data[4];                //4 byte , 'data'
    uint32_t data_size;             //4 byte ,
};




#include "onlinelist.h"
#endif
