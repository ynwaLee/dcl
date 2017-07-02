#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "rfu_client.h"
#include "conf_parser.h"
#include "../../common/applog.h"
#include "../../common/sock_clis.h"


#define RFU_SRV_RCV_HEADER   1
#define RFU_SRV_RCV_DATA     2

#define DCL_BUFSIZE         20480

unsigned char g_server_elmt_rfu;
unsigned char g_rfu_srv_rcv_phase;

object_clue_t object_clue_ids[MAX_OBJECT_CLUE];
unsigned int object_clue_counter;



unsigned int get_object(unsigned int clue)
{
    int i;

    for (i = 0; i < MAX_OBJECT_CLUE; i++) {
        if (object_clue_ids[i].flag == 1 && object_clue_ids[i].clue_id == clue) {
            return object_clue_ids[i].object_id;
        }
    }

    return 0;
}


int add_object_clue(unsigned int cmd, unsigned int object, unsigned int clue)
{
    int i;
    if (object_clue_counter > MAX_OBJECT_CLUE) {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "too many object and clue, %d:%d", object, clue);
        return -1;
    }

    if (cmd == 0) {
        for (i = 0; i < MAX_OBJECT_CLUE; i++) {
            if (object_clue_ids[i].clue_id == clue) {
                object_clue_ids[i].flag = 0;
                break;
            }
        }
        object_clue_counter -= 1;

    } else {
        for (i = 0; i < MAX_OBJECT_CLUE; i++) {
            if (object_clue_ids[i].clue_id == clue) {
                object_clue_ids[i].object_id = object;
                object_clue_ids[i].flag = 1;
                break;
            }
        }
        if (i == MAX_OBJECT_CLUE) {
            for (i = 0; i < MAX_OBJECT_CLUE; i++) {
                if (object_clue_ids[i].flag != 1) {
                    object_clue_ids[i].clue_id = clue;
                    object_clue_ids[i].object_id = object;
                    object_clue_ids[i].flag = 1;
                    break;
                }
            }
        }
        object_clue_counter += 1;
    }

    return 0;
}


static int rfu_srv_init(int fd)
{

    memset(object_clue_ids, 0, sizeof(object_clue_ids));
    object_clue_counter = 0;
    g_server_elmt[g_server_elmt_rfu].recv_len = 0;
    g_server_elmt[g_server_elmt_rfu].remain_len = 4;
    g_rfu_srv_rcv_phase = RFU_SRV_RCV_HEADER;

    return 0;
}

#define LOCAL_RULES_FILE    "/dev/shm/object_clue"
int write_rules_file(void)
{
    int i;
    FILE *fp;

    fp = fopen(LOCAL_RULES_FILE, "w+");

    for (i = 0; i < MAX_OBJECT_CLUE; i++) {
        if (object_clue_ids[i].flag == 1) {
            fprintf(fp, "object: %u\tclue: %u\n", object_clue_ids[i].object_id, object_clue_ids[i].clue_id);
        }
    }

    fclose(fp);

    return 0;
}



static int rfu_srv_recv(int fd)
{
    uint16_t pkt_len;
    uint8_t *data;
    uint16_t flag;
    uint16_t data_len;
    uint32_t cmd;
    uint32_t object_id;
    uint32_t clue_id;
    int i;

    data = g_server_elmt[g_server_elmt_rfu].recv_data;
    data_len = g_server_elmt[g_server_elmt_rfu].recv_len;
    applog(LOG_DEBUG, APP_LOG_MASK_BASE, "recv len: %d", data_len);

    if (g_rfu_srv_rcv_phase == RFU_SRV_RCV_HEADER)
    {
        flag = ntohs(*(uint16_t *)data);
        pkt_len = ntohs(*(uint16_t *)(data + 2));
        applog(LOG_DEBUG, APP_LOG_MASK_BASE, "header, flag: %x, pkt_len: %d", flag, pkt_len);

        if (flag != 0xffff || pkt_len > DCL_BUFSIZE || (pkt_len % 12 != 0)) {
            return -1;
        }

        if (pkt_len) {
            g_server_elmt[g_server_elmt_rfu].recv_len = 0;
            g_server_elmt[g_server_elmt_rfu].remain_len = pkt_len;
            g_rfu_srv_rcv_phase = RFU_SRV_RCV_DATA;
        } else {
            g_server_elmt[g_server_elmt_rfu].recv_len = 0;
            g_server_elmt[g_server_elmt_rfu].remain_len = 4;
            g_rfu_srv_rcv_phase = RFU_SRV_RCV_HEADER;
        }
    }
    else if (g_rfu_srv_rcv_phase == RFU_SRV_RCV_DATA)
    {
        for (i = 0; i < data_len; i += 12) {
            cmd = ntohl(*(uint32_t *)(data + i));
            object_id = ntohl(*(uint32_t *)(data + i + 4));
            clue_id = ntohl(*(uint32_t *)(data + i + 8));
            add_object_clue(cmd, object_id, clue_id);
            applog(LOG_DEBUG, APP_LOG_MASK_BASE, "cmd: %d, object id: %d, clue id: %d", cmd, object_id, clue_id);
        }
        write_rules_file();
        g_server_elmt[g_server_elmt_rfu].recv_len = 0;
        g_server_elmt[g_server_elmt_rfu].remain_len = 4;
        g_rfu_srv_rcv_phase = RFU_SRV_RCV_HEADER;
    }

    return 0;
}



static int rfu_srv_send(int fd)
{
    return 0;
}




void register_rfu_server(void)
{
    if (cfu_conf.rfu_port == 0)
    {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "No connect to rfu server. rfu port: %d", cfu_conf.rfu_port);
        return ;
    }

    g_server_elmt[g_server_elmt_num].fd = 0;
    g_server_elmt[g_server_elmt_num].state = SOCK_FD_STATE_CLOSE;
    strcpy(g_server_elmt[g_server_elmt_num].ip, cfu_conf.rfu_ip);
    g_server_elmt[g_server_elmt_num].port = cfu_conf.rfu_port;
    g_server_elmt[g_server_elmt_num].retry_interval = 1000;
    g_server_elmt[g_server_elmt_num].counter = 0;
    g_server_elmt[g_server_elmt_num].flag = 0;
    g_server_elmt[g_server_elmt_num].recv_data = malloc(DCL_BUFSIZE);
    g_server_elmt[g_server_elmt_num].init = rfu_srv_init;
    g_server_elmt[g_server_elmt_num].recv = rfu_srv_recv;
    g_server_elmt[g_server_elmt_num].send = rfu_srv_send;
    g_server_elmt_rfu = g_server_elmt_num;
    g_server_elmt_num++;
}


