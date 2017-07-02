#ifndef __SVM_ONLINE_H__
#define __SVM_ONLINE_H__

#include <pthread.h>
#include <stdint.h>
#include <time.h>



#define SVM_ONLINE_NUM      102400


typedef struct svm_online_node_ {
    uint64_t callid;
    int ruleid;
    time_t dialtime;
    char fromphone[32];
    char tophone[32];
    unsigned int userid;
    unsigned int voiceid;
    unsigned int percent;
} svm_online_node_t;

typedef struct svm_online_ {
    svm_online_node_t node[SVM_ONLINE_NUM];
    uint32_t counter;
    pthread_mutex_t mutex;
} svm_online_t;


extern svm_online_t svm_online;

void *thread_svm_online(void *arg);


#endif


