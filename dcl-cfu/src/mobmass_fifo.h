#ifndef __MOBMASS_FIFO_H__
#define __MOBMASS_FIFO_H__

#include <stdint.h>
#include <pthread.h>
#include "mobile_cdr.h"


#define     MAX_MOBMASS_NUM    102400

typedef struct mobmass_fifo_ {
    int in;
    int out;
    int count;
    mobile_mass_t node[MAX_MOBMASS_NUM];
    pthread_mutex_t mutex;
} mobmass_fifo_t;

extern mobmass_fifo_t g_mobmass_fifo;

void mobmass_fifo_init(void);
int mobmass_fifo_push(mobile_mass_t *mass, mobmass_fifo_t *fifo);
int mobmass_fifo_pop (mobile_mass_t *mass, mobmass_fifo_t *fifo);

#endif
