#ifndef __SVM_CDR_H__
#define __SVM_CDR_H__

#include <stdint.h>
#include <time.h>

#define SVM_MASS_VOICE      "mass_voice"
#define SVM_MASS_SMS        "mass_sms"


typedef struct voice_record_ {
    uint64_t callid;
    unsigned int ruleid;
    int datasrcid;
    int status;
    int path;
    time_t dialtime;
    time_t starttime;
    time_t endtime;
    int    duration;
    char fromphone[32];
    char tophone[32];
    int linklayerid;
    char inunit[32];
    char outunit[32];
    int recordflag;
    int callflag;
    int remarkflag;
    int itemflag;
    unsigned int userid;
    unsigned int voiceid;
    unsigned int percent;
    unsigned int stage;
    unsigned int vrsflag;
    int i1;
    int i2;
} voice_record_t;

typedef struct sms_record_ {
    uint64_t smsid;
    int ruleid;
    int datasrcid;
    int status;
    char action[16];
    time_t sendtime;
    time_t recvtime;
    char fromphone[32];
    char fromimsi[32];
    char tophone[32];
    char toimsi[32];
    char content[2048];
    int len;
    int recordflag;
    int remarkflag;
    int itemflag;
    int i1;
    int i2;
} sms_record_t;



void *thread_svm_cdr(void *arg);
int check_filetype(char *filename, char *type);


#endif
