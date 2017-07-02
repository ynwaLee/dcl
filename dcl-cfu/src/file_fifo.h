#ifndef __FILE_FIFO_H__
#define __FILE_FIFO_H__

#include <stdint.h>
#include <pthread.h>


#define     MAX_FILE_NUM    102400
#define     FILENAME_LEN    1024

typedef struct file_node_ {
    char filename[256];
    char file_transfer[512];
    uint8_t case_flag;
    uint8_t flag;
} file_node_t;

typedef struct file_fifo_ {
    int in;
    int out;
    int count;
    file_node_t node[MAX_FILE_NUM];
    pthread_mutex_t mutex;
} file_fifo_t;


extern file_fifo_t mass_file_fifo;
extern file_fifo_t case_file_fifo;


void file_fifo_init(void);
int file_fifo_push(file_node_t *node, file_fifo_t *fifo);
int file_fifo_pop(file_node_t *node, file_fifo_t *fifo);

#endif
