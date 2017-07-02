#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE   600
#endif
#include "lispr.h"
#include "mobmass_fifo.h" 
#include "lis-pr.pb-c.h"
#include <unistd.h>
#include "ma_cli.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "conf_parser.h"
#include <alloca.h>

#include "../../common/conf.h"
#include "../../common/applog.h"
#include "../../common/sock_clis.h"
#include "../../common/daemon.h"
#include "../../common/thread_flag.h"
#include "../../common/signal_catch.h"
#include <time.h>
#define    IMSI_LEN    16
#define    IMEI_LEN    16
#define    MSISDN_LEN    32
/* . read conf
 * . deal conf reload
 * . connect to lis-pr.
 * . pop fifo
 * . pack cdr
 * . send protobuf to lis-pr
 * */
extern unsigned char g_reload_flag;
extern int connect2server(char *ip, unsigned int port);

static int sock_recv(int fd, void *buf, unsigned int len)
{
    int recved = 0;
    int ret = 0;

    while (recved < len) {
        ret = recv(fd, buf + recved, len - recved, 0);
        if (ret <= 0) {
            return ret;
        }
        recved += ret;
    }

    return recved;
}

static int sock_send(int fd, void *buf, unsigned int len)
{
    int sended = 0;
    int ret = 0;

    while (sended < len) {
        ret = send(fd, buf + sended, len - sended, 0);
        if (ret <= 0) {
            return ret;
        }
        sended += ret;
    }

    return sended;
}

extern STRUCT_CFU_CONF_T cfu_conf;
static int get_lispr_info_from_cfu_yaml(char *ip, unsigned short *port)
{
    strcpy(ip, cfu_conf.remote_lis_ip);
    *port = cfu_conf.remote_lis_port;

    return 0;
}

static int proto_obj_set_value(LTERELATE__RELATEDATA *obj, mobile_mass_t *mass)
{
    if (obj == NULL || mass == NULL) {
        return -1;
    }

    obj->cmd = 1;

    obj->indextype = LTE__RELATE__INDEX__TYPE__BY_IMSI;

    if (mass->aimsi[0]) {
        obj->imsi = (char*)malloc(IMSI_LEN);
        if (obj->imsi == NULL) {
            return -1;
        }
        strncpy(obj->imsi, mass->aimsi, IMSI_LEN);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "imsi: %s", obj->imsi);
    }

    if (mass->aimei[0]) {
        obj->imei = (char*)malloc(IMEI_LEN);
        if (obj->imei == NULL) {
            return -1;
        }
        strncpy(obj->imei, mass->aimei, IMEI_LEN);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "imei: %s", obj->imei);
    }

    if (mass->aisdn[0]) {
        obj->msisdn = (char*)malloc(MSISDN_LEN);
        if (obj->msisdn == NULL) {
            return -1;
        }
        strncpy(obj->msisdn, mass->aisdn, MSISDN_LEN);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "isdn: %s", obj->msisdn);
    }

    unsigned short mcc;
    unsigned char  mnc;
    unsigned short lac;
    unsigned short cellid;

    obj->has_ecgi = 0;

    if (mass->amcc[0] != 0 && mass->amnc[0] != 0) {
        mcc = atoi(mass->amcc);
        mnc = atoi(mass->amnc);
        if (mass->alace[0] != 0 && mass->acelle != 0) {
            lac = atoi(mass->alace);
            cellid = atoi(mass->acelle);
            obj->has_ecgi = 1;
        }else if (mass->alacs[0] != 0 && mass->acells != 0) {
            lac = atoi(mass->alacs);
            cellid = atoi(mass->acells);
            obj->has_ecgi = 1;
        }
    }

    if (obj->has_ecgi) {
        char ecgi_buf[128] = {0};
        int len = snprintf(ecgi_buf, 128, "%03d%02d%04x%04x", mcc, mnc, lac, cellid);
        obj->ecgi.len = len + 1;
        obj->ecgi.data = (uint8_t *)malloc(obj->ecgi.len);
        memcpy(obj->ecgi.data, ecgi_buf, obj->ecgi.len);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "mcc: %d(%x), mnc: %d(%x), lac: %d(%x), cellid: %d(%x), ecgi_buf: %s",
                mcc, mcc, mnc, mnc, lac, lac, cellid, cellid, ecgi_buf);
    }    

    obj->has_datasource = 1;
    obj->datasource = LTE__RELATE__DATA__SOURCE__FROM_CS;

    obj->has_time = 0;
    char *ptime = NULL;
    struct tm tm = {0};
    time_t time = 0;
    unsigned int tmptime = 0;
    
    /* according to the action type. */
    if (mass->endtime[0] != 0) {
        ptime = mass->endtime;
    }else if (mass->starttime[0] != 0) {
        ptime = mass->starttime;
    }

    if (ptime != NULL) {
        strptime(ptime, "%Y/%m/%d %H:%M:%S", &tm);
        time = mktime(&tm);
        tmptime = (unsigned int)time;
        obj->has_time = 1;
        obj->time = (unsigned int)tmptime;
        applog(LOG_INFO, APP_LOG_MASK_BASE, "time: %lu, tmptime: %u, obj time: %u", time, tmptime, obj->time);
    }    

    obj->has_ret_value = 1;
    obj->ret_value = LTE__RELATE__RETURN__TYPE__UPDATE_SUCCESS;

    return 0;
}

static inline void proto_obj_destroy(LTERELATE__RELATEDATA *obj)
{
    if (obj == NULL) {
        return;
    }

    if (obj->imsi) {
        free(obj->imsi);
    }

    if (obj->imei) {
        free(obj->imei);
    }

    if (obj->msisdn) {
        free(obj->msisdn);
    }

    if (obj->ecgi.data) {
        free(obj->ecgi.data);
    }

    memset(obj, 0, sizeof(LTERELATE__RELATEDATA));
    return ;
}

static int pack_mobile_mass(mobile_mass_t *mass, void **proto_buf, unsigned int *proto_buf_len)
{
    void *buf; 
    unsigned len;

    LTERELATE__RELATEDATA proto_obj = LTE__RELATE__RELATE__DATA__INIT; 

    if (proto_obj_set_value(&proto_obj, mass) == -1) {
        proto_obj_destroy(&proto_obj);
        return -1;
    }

    len = lte__relate__relate__data__get_packed_size(&proto_obj);

    buf = calloc(1, len + 7);
    if (buf == NULL) {
        proto_obj_destroy(&proto_obj);
        return -1;
    }

    lte__relate__relate__data__pack(&proto_obj, buf + 7);

    *(unsigned char *)(buf + 0) = 0x4c;
    *(unsigned char *)(buf + 1) = 0x54;
    *(unsigned char *)(buf + 2)  = 0x45;

    *(uint32_t *)(buf + 3) = htonl(len);

    *proto_buf_len = len + 7;
    *proto_buf = buf;

    proto_obj_destroy(&proto_obj);

    return 0;
}

static int recv_operation_result(int lispr_fd)
{
    int retval = LISPR_RETVAL_ERROR_NONE;
    char *buf = alloca(7);

    if (sock_recv(lispr_fd, buf, 7) <= 0) {
        return LISPR_RETVAL_ERROR_CONN;    
    }
    applog(LOG_INFO, APP_LOG_MASK_BASE, "recv result header success. 7 bytes.");

    if (strncmp(buf, "LTE", 3) != 0) {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "recv lispr retval error: Not LTE.");
        return LISPR_RETVAL_ERROR_UPDATE;
    }

    buf += 3;
    int len = ntohl(*(int *)buf);


    LTERELATE__RELATEDATA *proto_obj = NULL;
    uint8_t *proto_buf = alloca(len);
    if (sock_recv(lispr_fd, proto_buf, len) <= 0) {
        return LISPR_RETVAL_ERROR_CONN;    
    }

    proto_obj = lte__relate__relate__data__unpack(NULL, len, proto_buf);
    if (proto_obj == NULL) {
        return LISPR_RETVAL_ERROR_UPDATE;    
    }
    if (proto_obj->has_ret_value) {
        retval = proto_obj->ret_value;
    }
    lte__relate__relate__data__free_unpacked(proto_obj, NULL);

    return retval;
}
static void show_mobile_mass_cdr(mobile_mass_t *mass)
{
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, 
            "aisdn: %s, ahome: %s, aimsi: %s, aimei: %s, aneid: %s, amcc: %s, amnc: %s, alacs: %s, acells: %s, alace: %s, acelle: %s, apc: %s, bisdn: %s, bisdnx: %s, bhome: %s, btisdn: %s, bimsi: %s, bneid: %s, bpc: %s, acttype: %s, starttime: %s, endtime: %s, duration: %s, vocfile: %s, extinfo: %s, result: %s, cause: %s, netid: %s, smslan: %s, smstext: %s", 
            mass->aisdn,
            mass->ahome,
            mass->aimsi,
            mass->aimei,
            mass->aneid,
            mass->amcc,
            mass->amnc,
            mass->alacs,
            mass->acells,
            mass->alace,
            mass->acelle,
            mass->apc,
            mass->bisdn,
            mass->bisdnx,
            mass->bhome,
            mass->btisdn,
            mass->bimsi,
            mass->bneid,
            mass->bpc,
            mass->acttype,
            mass->starttime,
            mass->endtime,
            mass->duration,
            mass->vocfile,
            mass->extinfo,
            mass->result,
            mass->cause,
            mass->netid,
            mass->smslan,
            mass->smstext);
}

static int send_proto_buf(int lispr_fd, void *proto_buf, int proto_buf_len)
{
    int data_error_send_times = 0;

    for (;;) {
        /* send protobuf to lis-pr */
        if (sock_send(lispr_fd, proto_buf, proto_buf_len) <= 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "send proto buf to lispr error.");
            return -1;
        }
        applog(LOG_INFO, APP_LOG_MASK_BASE, "send proto buf data to LIS-PR success.");

        /* recv the operation result */
        int ret = recv_operation_result(lispr_fd);
        if (ret == LISPR_RETVAL_ERROR_DATA) {     //data format error, use the right cmd, index
            applog(LOG_INFO, APP_LOG_MASK_BASE, "result: data format error. check the cmd=1, index=3");
            if (data_error_send_times < 3) {
                data_error_send_times++;
                continue;
            }
            break;
        }else if (ret == LISPR_RETVAL_ERROR_UPDATE) { //update operation error, srv checks the imsi
            applog(LOG_INFO, APP_LOG_MASK_BASE, "result: update operation error. the imsi is Null.");
            break;
        }else if (ret == LISPR_RETVAL_ERROR_NONE) {  //success
            applog(LOG_INFO, APP_LOG_MASK_BASE, "result: success.");
            break;
        }else if (ret == LISPR_RETVAL_ERROR_CONN) {
            return -1;
        }
    }

    return 0;
}

void *thread_lispr(void *argv)
{
    mobile_mass_t mass = {{0}};

    char lispr_ip[16] = {0};
    unsigned short lispr_port = 0;
    int lispr_fd = 0;

    void *proto_buf = NULL;
    unsigned int proto_buf_len = 0;

    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }

        /* read cfu conf file */
        if (lispr_ip[0] == 0) {
            if (get_lispr_info_from_cfu_yaml(lispr_ip, &lispr_port) == -1) {
                applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "get lispr info error.");
                sleep(1);
                continue;
            }
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "get lispr info: %s, %d.", lispr_ip, lispr_port);
        }

        /* reload cfu conf file */
        if (g_reload_flag & CONF_RELOAD_CFU) {
            if (lispr_fd != 0) {
                close(lispr_fd);
                lispr_fd = 0;
            }
            memset(lispr_ip, 0, 16);
            lispr_port = 0;
            continue;
        }

        /* connect to lis pr */
        if (lispr_fd == 0) {
            if ((lispr_fd = connect2server(lispr_ip, lispr_port)) == -1) {
                applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "connect to lis pr %s, %d error.", lispr_ip, lispr_port);
                lispr_fd = 0;
                sleep(1);
                continue;
            }
        }

        /* pop a mobile mass cdr when imsi is null. Maybe imsi is not null when connection error. */
        if (proto_buf == NULL) {
            /* pop a mobile mass cdr */
            if (mobmass_fifo_pop(&mass, &g_mobmass_fifo) == -1) {
                usleep(1000*10);
                continue;
            }

            /* show the mass mobile cdr */
            show_mobile_mass_cdr(&mass);

            /* check the imsi in mass, imsi is the index which must't be null*/
            if (mass.aimsi[0] == 0) {
                applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "imsi is null, drop it.");
                continue;
            }

            /* pack cdr */
            if (pack_mobile_mass(&mass, &proto_buf, &proto_buf_len) == -1) {
                applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "pack mobile mass error.");
                continue;    
            }
        }

        /* send proto buf  */
        if (send_proto_buf(lispr_fd, proto_buf, proto_buf_len) == -1) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "connection error, reconnect to lispr.");
            close(lispr_fd);
            lispr_fd = 0;
            continue;
        }


        free(proto_buf);
        proto_buf = NULL;
        proto_buf_len = 0;
    }

    return NULL;
}
