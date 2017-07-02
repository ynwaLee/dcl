#ifndef __MOBONLINE_ARRAY_H__
#define __MOBONLINE_ARRAY_H__

#include <stdint.h>
#include <pthread.h>
#include "mobile_online.h"

#define SYSTYPE_MOBILE  2
#define SYSTYPE_VOIPA   3
#define SYSTYPE_VOIPB   4

#define     MAX_MOBONLINE_NUM    102400

typedef struct mobonline_array_ {
    int count;
    mobile_online_t node[MAX_MOBONLINE_NUM];
    pthread_mutex_t mutex;
} mobonline_array_t;

extern mobonline_array_t g_mobonline_array;

void mobonline_array_init(void);
int mobonline_array_add(mobile_online_t *online, mobonline_array_t *array);
int mobonline_array_remove(mobonline_array_t *array, unsigned int systype, unsigned int mntid, unsigned int channel);
#endif
