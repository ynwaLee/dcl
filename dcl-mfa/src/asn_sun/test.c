#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include "asn.h"


#define PKT_SIZE    10485760
typedef struct realtime_voice_s {
	unsigned char mntid[128];   //MNTID
	unsigned int channel; //通信识别码
	unsigned int vocsequence; //序号
	unsigned char voctime[20];   //时间戳, 字符串形式YYYYMMDDHHMMSS
	unsigned char vocdirect; //1-up, 2-down, 3-不区分
	unsigned char vocencode; //0x6E-EFR, 0x70-AMR, 0x08-PCM
	unsigned int voclen;
	unsigned char *vocdata;
	unsigned char vocframe; //帧类型，指上面的“语音数据”的帧格式，0-IP帧；1-UDP帧；2-RTP帧；3-语音帧。一般都是3，即纯语音数据
}realtime_voice_t; 


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
		if (res && contentlen <= 8) {
			memcpy(voice->mntid, content, contentlen);
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


int main(void)
{
    FILE * fpasn = fopen("hi3.dat", "rb");
    FILE * fptxt = fopen("hi3.txt", "w+");

    unsigned char buf[PKT_SIZE];
    unsigned char * pkt = buf;
    int len = fread(pkt, 1, PKT_SIZE, fpasn);
    fclose(fpasn);
	realtime_voice_t *voice = alloca(sizeof(realtime_voice_t));
	memset(voice, 0, sizeof(realtime_voice_t));

    int os = 0;
    while(len > 0)
    {
        printf("while loop\n");
        unsigned int layer[32];
        int asnlen = deasn(pkt+os, len, voice, layer, 1, 32, asncallback);
        os += asnlen;
        len -= asnlen;
    }
    fclose(fptxt);
	
#if 1
	{
		printf("mntid: %s, channel: %X, vocsequence: %X, voctime: %s, vocdirect: %X, vocencode: %X, voclen: %X, vocframe: %X\n", 
				voice->mntid,
				voice->channel,
				voice->vocsequence,
				voice->voctime,
				voice->vocdirect,
				voice->vocencode,
				voice->voclen,
				voice->vocframe);
		int i = 0;
		for (; i < voice->voclen && voice->vocdata != NULL; i++) {
			printf("%02X ", voice->vocdata[i]);
		}
		printf("\n");
	}
#endif
    return 0;
}
