#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "file_fifo.h"
#include "cfu.h"
#include "../../common/applog.h"

file_fifo_t mass_file_fifo;
file_fifo_t case_file_fifo;



void file_fifo_init(void)
{
    memset(&mass_file_fifo, 0, sizeof(file_fifo_t));
    pthread_mutex_init(&mass_file_fifo.mutex, NULL);
    memset(&case_file_fifo, 0, sizeof(file_fifo_t));
    pthread_mutex_init(&case_file_fifo.mutex, NULL);
}


int file_fifo_push(file_node_t *node, file_fifo_t *fifo)
{
    int in;

    if (node == NULL || fifo == NULL) {
        return -1;
    }
    pthread_mutex_lock(&fifo->mutex);

    in = fifo->in;
    if (fifo->node[in].flag == 1) {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "%s cover %s in fifo", node->filename, fifo->node[in].filename);
    }
    strcpy(fifo->node[in].filename, node->filename);
    strcpy(fifo->node[in].file_transfer, node->file_transfer);
    fifo->node[in].flag = 1;
    fifo->node[in].case_flag = node->case_flag;
    fifo->in = in + 1;
    fifo->count += 1;
    if (fifo->in >= MAX_FILE_NUM) {
        fifo->in = 0;
    }
    if (fifo->count > MAX_FILE_NUM) {
        fifo->count = MAX_FILE_NUM;
        applog(LOG_ERR, APP_LOG_MASK_BASE, "too much bcp in fifo, more than %d", MAX_FILE_NUM);
    }

    if (fifo->count > MAX_FILE_NUM / 5 * 4) {
        applog(LOG_INFO, APP_LOG_MASK_BASE, "%d bcp in %d fifo", fifo->count, MAX_FILE_NUM);
    }

    pthread_mutex_unlock(&fifo->mutex);

    return 1;
}

int file_fifo_pop(file_node_t *node, file_fifo_t *fifo)
{
    int return_value = -1;
    int out;

    if (node == NULL || fifo == NULL) {
        return -1;
    }

    pthread_mutex_lock(&fifo->mutex);

    if (fifo->count > 0) {
        out = fifo->out;
        if (fifo->node[out].flag == 1) {
            strcpy(node->filename, fifo->node[out].filename);
            strcpy(node->file_transfer, fifo->node[out].file_transfer);
            node->case_flag = fifo->node[out].case_flag;
        }
        return_value = fifo->node[out].flag;
        fifo->node[out].flag = 0;
        fifo->out = out + 1;
        if (fifo->count > 0) {
            fifo->count -= 1;
        }
        if (fifo->out >= MAX_FILE_NUM) {
            fifo->out = 0;
        }
    } else {
        return_value = 0;
    }

    pthread_mutex_unlock(&fifo->mutex);

    return return_value;
}





