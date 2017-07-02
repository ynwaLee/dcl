#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "mobonline_array.h"

mobonline_array_t g_mobonline_array;



void mobonline_array_init(void)
{
    memset(&g_mobonline_array, 0, sizeof(mobonline_array_t));
    pthread_mutex_init(&g_mobonline_array.mutex, NULL);
}


int mobonline_array_add(mobile_online_t *online, mobonline_array_t *array)
{
    int ret = -1;
    int i = 0;
    mobile_online_t zero_online = {0};

    if (online == NULL || array == NULL) {
        return -1;
    }

    pthread_mutex_lock(&array->mutex);
    if (array->count < MAX_MOBONLINE_NUM) {
        for (i = 0; i < MAX_MOBONLINE_NUM; i++) {
            if (bcmp(&array->node[i], &zero_online, sizeof(mobile_online_t)) == 0) {
                memmove(&array->node[i], online, sizeof(mobile_online_t));    
                array->count++;
                ret = 0;
                break;
            }
        }
    }
    pthread_mutex_unlock(&array->mutex);

    return ret;
}

int mobonline_array_remove(mobonline_array_t *array, unsigned int systype, unsigned int mntid, unsigned int channel)
{
    int i = 0;
    pthread_mutex_lock(&array->mutex);
    for (i = 0; i < MAX_MOBONLINE_NUM; i++) {
        if (array->node[i].systype == systype && array->node[i].mntid == mntid && array->node[i].channel == channel) {
            bzero(&array->node[i], sizeof(mobile_online_t));
            array->count--;
            break;
        }
    }
    pthread_mutex_unlock(&array->mutex);
    return 0;
}
