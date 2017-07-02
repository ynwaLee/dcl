#ifndef __MOBILE_ONLINE_H__
#define    __MOBILE_ONLINE_H__

#include <time.h>

void *thread_mobile_online(void *arg);
int check_data_complete(unsigned char *data, uint32_t len);
void asncallback(unsigned int tag, int constructed, unsigned char * content, int contentlen, unsigned int *layer, int curlayer, void * para);

typedef struct mobile_online_s {
    time_t recv_time;
    unsigned int systype;
    unsigned int mntid;         //1.
    char operator[64];
    unsigned int channel;        //2.
    time_t time;
    unsigned int calldir;
    int  mcc;
    int  mnc;
    int  lac;
    int  cellid;
    int  aaction;       // 0. calling, 1. called
    char aimei[64];
    char aimsi[64];
    char amsisdn[64];
    int  baction;       // 0. calling, 1. called
    char bphonenum[64];
    char smscontent[1024];
    char signal_action[64];
    /*"call-establishment", "answer", "supplementary-Service", "handover", "release", "sMS", "location-update", "subscriber-Controlled-Input", "periodic-update", "power-on", "power-off", "authenticate", "paging"*/
} mobile_online_t;

#endif
