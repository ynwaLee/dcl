#include "onlinelist.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "amrcode.h"
#include "common_header.h"
#include "g729_decoder.h"

void online_list_init(online_list_t *list)
{
    list->header = calloc(1, sizeof(online_node_t));
    pthread_mutex_init(&list->mutex, NULL);
    list->header->next = list->header;
    list->header->prev = list->header;
}

void online_list_append(online_list_t *list, online_node_t *node)
{
    pthread_mutex_lock(&list->mutex);
    online_node_t *header = list->header;
    online_node_t *tail = header->prev;

    node->prev = tail;
    node->next = header;

    tail->next = node;
    header->prev = node;
    pthread_mutex_unlock(&list->mutex);

    return;
}

int online_list_timeout_check(online_list_t *list)
{
    online_node_t *curr = NULL;
    time_t nowtime = time(0);
    pthread_mutex_lock(&list->mutex);
    for (curr = list->header->next; curr != list->header; curr = curr->next) {
        if (nowtime - curr->last_modify_time > 8) {
            online_node_t *prev = curr->prev;
            online_node_t *next = curr->next;
            prev->next = next;
            next->prev = prev;
            online_node_free(curr);
            curr = prev;
        }
    }
    pthread_mutex_unlock(&list->mutex);
    return 0;
}

void online_list_remove(online_list_t *list, online_node_t *node)
{
    if (online_list_is_empty(list)) {
        return;
    }

    if (node == list->header) {
        return;
    }

    pthread_mutex_lock(&list->mutex);
    online_node_t *prev = node->prev;
    online_node_t *next = node->next;

    prev->next = next;
    next->prev = prev;
    pthread_mutex_unlock(&list->mutex);

    return;
}

online_node_t *online_list_find(online_list_t *list, int systemtypes,unsigned long long mntid, unsigned int channel)
{
    online_node_t *curr = NULL;
    pthread_mutex_lock(&list->mutex);
    for (curr = list->header->next; curr != list->header; curr = curr->next) {
        if (curr->mntid == mntid && curr->channel == channel&&curr->systypes == systemtypes) {
            curr->last_modify_time = time(0);
            pthread_mutex_unlock(&list->mutex);
            return curr;
        }
    }
    pthread_mutex_unlock(&list->mutex);

    return NULL;
}

void online_list_destroy(online_list_t *list)
{
    online_list_clear(list);
    free(list->header);
    pthread_mutex_destroy(&list->mutex);
}

bool online_list_is_empty(online_list_t *list)
{
    bool r = false;
    pthread_mutex_lock(&list->mutex);
    if (list->header->next == list->header) {
        r = true;
    }
    pthread_mutex_unlock(&list->mutex);

    return r;
}

void online_list_clear(online_list_t *list)
{
    pthread_mutex_lock(&list->mutex);
    online_node_t *curr, *next;
    for (curr = list->header->next; curr != list->header; curr = next) {
        next = curr->next;
        free(curr);
    }
    list->header->next = list->header;
    list->header->prev = list->header;
    pthread_mutex_unlock(&list->mutex);
}


realtime_voice_t *realtime_voice_calloc(void)
{
    realtime_voice_t *voice = calloc(1, sizeof(realtime_voice_t));
    return voice;
}
realtime_voice_t *realtime_voice_dup(realtime_voice_t *voice)
{
    if (voice == NULL) {
        return NULL;
    }

    realtime_voice_t *pvoc = calloc(1, sizeof(realtime_voice_t));
    memcpy(pvoc, voice, sizeof(realtime_voice_t));

    if (voice->vocdata != NULL) {
        pvoc->vocdata = calloc(1, voice->voclen);
        memcpy(pvoc->vocdata, voice->vocdata, voice->voclen);
    }

    return pvoc;
}
void realtime_voice_free(realtime_voice_t *voice)
{
    if (voice == NULL) {
        return;
    }

    if (voice->vocdata != NULL) {
        free(voice->vocdata);
    }

    free(voice);
}

void online_node_free(online_node_t *node)
{
    if (node == NULL) {
        return;
    }

    if (node->sockfd != -1) {
        close(node->sockfd);
        Decoder_Interface_exit(node->destate[0]);
        Decoder_Interface_exit(node->destate[1]);
        G729DecClose(node->destate1[0]); 
        G729DecClose(node->destate1[1]);
    }

    if (node->last_buff) {
        free(node->last_buff);
        node->last_buff = NULL;
    }

    applog(LOG_INFO, APP_LOG_MASK_BASE, "release a node: mntid: %d, channel: %d", node->mntid, node->channel);
    free(node);
    return ;
}

