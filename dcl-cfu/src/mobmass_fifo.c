#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "mobmass_fifo.h"

mobmass_fifo_t g_mobmass_fifo;



void mobmass_fifo_init(void)
{
    memset(&g_mobmass_fifo, 0, sizeof(mobmass_fifo_t));
    pthread_mutex_init(&g_mobmass_fifo.mutex, NULL);
}


int mobmass_fifo_push(mobile_mass_t *mass, mobmass_fifo_t *fifo)
{
    int ret = 0;
    if (mass == NULL || fifo == NULL) {
        return -1;
    }
    pthread_mutex_lock(&fifo->mutex);
    if (fifo->count < MAX_MOBMASS_NUM) {
        memmove(&fifo->node[fifo->in], mass, sizeof(mobile_mass_t));    
        fifo->count++;
        fifo->in++;
        fifo->in = (fifo->in < MAX_MOBMASS_NUM) ? fifo->in : 0;
    }else {
        ret = -1;
    }
    pthread_mutex_unlock(&fifo->mutex);

    return ret;
}

int mobmass_fifo_pop(mobile_mass_t *mass, mobmass_fifo_t *fifo)
{
    int ret = 0;
    if (mass == NULL || fifo == NULL) {
        return -1;
    }

    pthread_mutex_lock(&fifo->mutex);
    if (fifo->count > 0) {
        memmove(mass, &fifo->node[fifo->out], sizeof(mobile_mass_t));
        fifo->count--;
        fifo->out++;
        fifo->out = (fifo->out < MAX_MOBMASS_NUM) ? fifo->out : 0;
    }else {
        ret = -1;
    }
    pthread_mutex_unlock(&fifo->mutex);

    return ret;
}





