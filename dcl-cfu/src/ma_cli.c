#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cfu.h"
#include "conf_parser.h"
#include "ma_cli.h"
#include "counter.h"
#include "../../common/applog.h"
#include "../../common/sock_clis.h"
#include "../../common/ma_pkt.h"
#include "../../common/thread_flag.h"
#include "../../common/file_func.h"

#define DCL_BUFSIZE     2048

//extern STRUCT_SPU g_spu_para;
extern time_t g_time;

STRUCT_PKT_COUNTER g_worker_counter;
static time_t g_sguard_last_keepalive = 0;
MA_INTF_CONF ma_intf_conf;
unsigned char g_counter_struct_flag = 0;
unsigned char g_counter_flag = 0;
unsigned short g_counter_sn = 0;
unsigned char g_server_elmt_sguard;
unsigned char g_server_elmt_ma;

unsigned char g_reload_flag = CONF_RELOAD_NULL;


static uint16_t get_status(void)
{
    uint16_t status;

    status = 1;

    return status;
}


static int sguard_srv_init(int fd)
{
    uint8_t pkt[DCL_BUFSIZE];
    uint32_t pkt_len;
    int ret;

    g_server_elmt[g_server_elmt_sguard].recv_len = 0;
    g_server_elmt[g_server_elmt_sguard].remain_len = 0;

    pkt_len = fill_regsguard_packet(pkt, ma_intf_conf.pid, ma_intf_conf.sn, 0, 0);
    ret = sock_send_buf(fd, pkt, pkt_len);
    if (ret)
        return -1;

    applog(LOG_INFO, APP_LOG_MASK_MA, "send regist info %d bytes to sguard", pkt_len);
    return 0;
}

static int sguard_srv_recv(int fd)
{
    int ret;
    uint8_t pkt[DCL_BUFSIZE];

    ret = read(fd, pkt, DCL_BUFSIZE);
    if (ret < 0)
        return -1;
    return 0;
}


static int sguard_srv_send(int fd)
{
    int ret;
    uint8_t pkt[DCL_BUFSIZE];
    uint32_t pkt_len;
    uint16_t status;

    if (g_sguard_last_keepalive == g_time)
        return 0;

    status = get_status();
    pkt_len = fill_alive_packet(pkt, status);
    ret = sock_send_buf(fd, pkt, pkt_len);
    if (ret)
        return -1;

    g_sguard_last_keepalive = g_time;
    return 0;
}


void register_sguard_server(void)
{
    if (ma_intf_conf.port_sguard == 0 || ma_intf_conf.port_ma == 0 || ma_intf_conf.sn == 0)
    {
        applog(LOG_ERR, APP_LOG_MASK_MA, "No connect to sguard server. sguard port: %d, ma port: %d, sn: %d",
                ma_intf_conf.port_sguard, ma_intf_conf.port_ma, ma_intf_conf.sn);
        return ;
    }

    g_server_elmt[g_server_elmt_num].fd = 0;
    g_server_elmt[g_server_elmt_num].state = SOCK_FD_STATE_CLOSE;
    strcpy(g_server_elmt[g_server_elmt_num].ip, ma_intf_conf.ip_sguard);
    g_server_elmt[g_server_elmt_num].port = ma_intf_conf.port_sguard;
    g_server_elmt[g_server_elmt_num].retry_interval = 1000;
    g_server_elmt[g_server_elmt_num].counter = 0;
    g_server_elmt[g_server_elmt_num].flag = 0;
    g_server_elmt[g_server_elmt_num].recv_data = NULL;
    g_server_elmt[g_server_elmt_num].init = sguard_srv_init;
    g_server_elmt[g_server_elmt_num].recv = sguard_srv_recv;
    g_server_elmt[g_server_elmt_num].send = sguard_srv_send;
    g_server_elmt_sguard = g_server_elmt_num;
    g_server_elmt_num++;
}



extern unsigned char g_reload_flag;


static int ma_cmd_reload(uint8_t *data, uint32_t data_len)
{
    unsigned char flag = 0;
    char *conf[5];
    uint8_t num, i;

    num = get_ma_reload_conf(data, data_len, &conf[0], 5);
    for (i = 0; i < num; i++)
    {
        applog(LOG_INFO, APP_LOG_MASK_MA, "Reload conf file %u: %s", i, conf[i]);
        if (strcmp(conf[i], "cfu.yaml")==0)
        {
            flag |= CONF_RELOAD_CFU;
        }
        else if (strcmp(conf[i], "dms.yaml")==0)
        {
            flag |= CONF_RELOAD_DMS;
        }
        else if (strcmp(conf[i], "all")==0)
        {
            flag = CONF_RELOAD_ALL;
        }
        else
        {
            applog(LOG_WARNING, APP_LOG_MASK_MA, "cfu can not read file name!");
            return 0;
        }
    }

    g_reload_flag = flag;
    applog(LOG_INFO, APP_LOG_MASK_MA, "reload flag %x",g_reload_flag);

    return 0;
}



static int ma_cmd_send_counter(int sockfd)
{
    int ret;
    uint8_t pkt_buf[1500];
    uint16_t pkt_len;

    //applog(LOG_DEBUG, APP_LOG_MASK_MA, "CFU counter");

    g_worker_counter.svm_mass_file_num = atomic64_read(&svm_mass_file_num);
    g_worker_counter.svm_mass_call_num = atomic64_read(&svm_mass_call_num);
    g_worker_counter.svm_case_file_num = atomic64_read(&svm_case_file_num);
    g_worker_counter.svm_case_call_num = atomic64_read(&svm_case_call_num);
    g_worker_counter.svm_file_num = g_worker_counter.svm_mass_file_num + g_worker_counter.svm_case_file_num;
    g_worker_counter.svm_call_num = g_worker_counter.svm_mass_call_num + g_worker_counter.svm_case_call_num;
    g_worker_counter.mobile_mass_file_num = atomic64_read(&mobile_mass_file_num);
    g_worker_counter.mobile_mass_call_num = atomic64_read(&mobile_mass_call_num);
    g_worker_counter.mobile_case_file_num = atomic64_read(&mobile_case_file_num);
    g_worker_counter.mobile_case_call_num = atomic64_read(&mobile_case_call_num);
    g_worker_counter.mobile_file_num = g_worker_counter.mobile_mass_file_num + g_worker_counter.mobile_case_file_num;
    g_worker_counter.mobile_call_num = g_worker_counter.mobile_mass_call_num + g_worker_counter.mobile_case_call_num;
    g_worker_counter.das_mass_file_num = atomic64_read(&das_mass_file_num);
    g_worker_counter.das_mass_call_num = atomic64_read(&das_mass_call_num);
    g_worker_counter.das_case_file_num = atomic64_read(&das_case_file_num);
    g_worker_counter.das_case_call_num = atomic64_read(&das_case_call_num);
    g_worker_counter.das_file_num = g_worker_counter.das_mass_file_num + g_worker_counter.das_case_file_num;
    g_worker_counter.das_call_num = g_worker_counter.das_mass_call_num + g_worker_counter.das_case_call_num;
    g_worker_counter.das_upload_success = atomic64_read(&das_upload_success);

    g_worker_counter.svm_mass_file_num = htobe64(g_worker_counter.svm_mass_file_num);
    g_worker_counter.svm_mass_call_num = htobe64(g_worker_counter.svm_mass_call_num);
    g_worker_counter.svm_case_file_num = htobe64(g_worker_counter.svm_case_file_num);
    g_worker_counter.svm_case_call_num = htobe64(g_worker_counter.svm_case_call_num);
    g_worker_counter.svm_file_num = htobe64(g_worker_counter.svm_file_num);
    g_worker_counter.svm_call_num = htobe64(g_worker_counter.svm_call_num);
    g_worker_counter.mobile_mass_file_num = htobe64(g_worker_counter.mobile_mass_file_num);
    g_worker_counter.mobile_mass_call_num = htobe64(g_worker_counter.mobile_mass_call_num);
    g_worker_counter.mobile_case_file_num = htobe64(g_worker_counter.mobile_case_file_num);
    g_worker_counter.mobile_case_call_num = htobe64(g_worker_counter.mobile_case_call_num);
    g_worker_counter.mobile_file_num = htobe64(g_worker_counter.mobile_file_num);
    g_worker_counter.mobile_call_num = htobe64(g_worker_counter.mobile_call_num);
    g_worker_counter.das_mass_file_num = htobe64(g_worker_counter.das_mass_file_num);
    g_worker_counter.das_mass_call_num = htobe64(g_worker_counter.das_mass_call_num);
    g_worker_counter.das_case_file_num = htobe64(g_worker_counter.das_case_file_num);
    g_worker_counter.das_case_call_num = htobe64(g_worker_counter.das_case_call_num);
    g_worker_counter.das_file_num = htobe64(g_worker_counter.das_file_num);
    g_worker_counter.das_call_num = htobe64(g_worker_counter.das_call_num);
    g_worker_counter.das_upload_success = htobe64(g_worker_counter.das_upload_success);

    pkt_len = fill_ma_counter2(pkt_buf, 20, g_counter_sn, (uint8_t*)&g_worker_counter, sizeof(STRUCT_PKT_COUNTER));

    ret = sock_send_buf(sockfd, pkt_buf, pkt_len);
    if (ret) {
        applog(LOG_ERR, APP_LOG_MASK_MA, "write counter data less than pkt_len %dbytes", pkt_len);
        return -1;
    }

    return 0;
}



static int ma_cmd_send_counter_struct(int sockfd)
{
    int ret;
    uint8_t pkt_buf[2048];
    uint16_t pkt_len;

    struct count_struct field_array[19];

    applog(LOG_INFO, APP_LOG_MASK_MA, "CFU counter struct cmd");


    strncpy(field_array[0].name, "svm_file_num", 31);
    field_array[0].len = 8;
    strncpy(field_array[1].name, "svm_call_num", 31);
    field_array[1].len = 8;
    strncpy(field_array[2].name, "svm_mass_file_num", 31);
    field_array[2].len = 8;
    strncpy(field_array[3].name, "svm_mass_call_num", 31);
    field_array[3].len = 8;
    strncpy(field_array[4].name, "svm_case_file_num", 31);
    field_array[4].len = 8;
    strncpy(field_array[5].name, "svm_case_call_num", 31);
    field_array[5].len = 8;
    strncpy(field_array[6].name, "mobile_file_num", 31);
    field_array[6].len = 8;
    strncpy(field_array[7].name, "mobile_call_num", 31);
    field_array[7].len = 8;
    strncpy(field_array[8].name, "mobile_mass_file_num", 31);
    field_array[8].len = 8;
    strncpy(field_array[9].name, "mobile_mass_call_num", 31);
    field_array[9].len = 8;
    strncpy(field_array[10].name, "mobile_case_file_num", 31);
    field_array[10].len = 8;
    strncpy(field_array[11].name, "mobile_case_call_num", 31);
    field_array[11].len = 8;
    strncpy(field_array[12].name, "das_file_num", 31);
    field_array[12].len = 8;
    strncpy(field_array[13].name, "das_call_num", 31);
    field_array[13].len = 8;
    strncpy(field_array[14].name, "das_mass_file_num", 31);
    field_array[14].len = 8;
    strncpy(field_array[15].name, "das_mass_call_num", 31);
    field_array[15].len = 8;
    strncpy(field_array[16].name, "das_case_file_num", 31);
    field_array[16].len = 8;
    strncpy(field_array[17].name, "das_case_call_num", 31);
    field_array[17].len = 8;
    strncpy(field_array[18].name, "das_upload_success", 31);
    field_array[18].len = 8;


    pkt_len = fill_ma_count_info_struct(pkt_buf, g_counter_sn, ma_intf_conf.app_name, ma_intf_conf.app_name, 19, field_array);

    ret = sock_send_buf(sockfd, pkt_buf, pkt_len);
    if (ret) {
        applog(LOG_ERR, APP_LOG_MASK_MA, "write counter data less than pkt_len %dbytes", pkt_len);
        return -1;
    }

    return 0;
}



static int ma_cmd_parser(uint16_t cmd, uint16_t sn, uint8_t *pkt, uint16_t pkt_len)
{
    unsigned int v;
    int ret;
    uint8_t pkt_buf[32];

    switch(cmd) {
        case RELOAD:
            applog(LOG_INFO, APP_LOG_MASK_MA, "get reload config file cmd from ma");
            ma_cmd_reload(pkt, pkt_len);
            break;
        case GET_COUNTER:
            applog(LOG_DEBUG, APP_LOG_MASK_MA, "get counter cmd from ma");
            g_counter_flag = THREAD_OPR_FLAG_REQ;
            g_counter_sn = sn;
            break;
        case COUNTER_STRUCT:
            applog(LOG_DEBUG, APP_LOG_MASK_MA, "get counter struct cmd from ma");
            g_counter_struct_flag = THREAD_OPR_FLAG_REQ;
            g_counter_sn = sn;
            break;
        case DEBUG_ON:
            applog_debug_on(&(g_server_elmt[g_server_elmt_ma].fd));
            break;
        case DEBUG_OFF:
            applog_debug_off();
            break;
        case SYSLOG_LEVEL:
            ma_cmd_get_loglevel(pkt, pkt_len, &v);
            applog_set_log_level(v);
            break;
        case DEBUG_MASK:
            ma_cmd_get_logmask(pkt, pkt_len, &v);
            applog_set_debug_mask(v);
            break;
        case SHOW_SYS_LEV:
            ret = fill_loglevel_packet(pkt_buf, sn, g_app_log_level);
            sock_send_buf(g_server_elmt[g_server_elmt_ma].fd, pkt_buf, ret);
            break;
        case SHOW_DEBUG_MASK:
            ret = fill_logmask_packet(pkt_buf, sn, g_app_debug_mask);
            sock_send_buf(g_server_elmt[g_server_elmt_ma].fd, pkt_buf, ret);
            break;
        default:
            break;
    }
    return 0;
}

#define MA_SRV_RCV_HEADER    1
#define MA_SRV_RCV_DATA        2

unsigned char g_ma_srv_rcv_phase;
unsigned short g_ma_srv_rcv_cmd;
unsigned short g_ma_srv_rcv_sn;


static int ma_srv_init(int fd)
{
    uint8_t pkt[DCL_BUFSIZE];
    uint32_t pkt_len;
    int ret;

    g_server_elmt[g_server_elmt_ma].recv_len = 0;
    g_server_elmt[g_server_elmt_ma].remain_len = sizeof(struct pkt_header);
    g_ma_srv_rcv_phase = MA_SRV_RCV_HEADER;

    pkt_len = fill_regma_packet(pkt, ma_intf_conf.pid, ma_intf_conf.sn);
    ret = sock_send_buf(fd, pkt, pkt_len);
    if (ret)
        return -1;
    applog(LOG_INFO, APP_LOG_MASK_MA, "send regist info %d bytes to ma", pkt_len);
    return 0;
}

static int ma_srv_recv(int fd)
{
    uint16_t pkt_len;

    if (g_ma_srv_rcv_phase == MA_SRV_RCV_HEADER)
    {
        ma_header_get_cmd_sn_len(g_server_elmt[g_server_elmt_ma].recv_data, &g_ma_srv_rcv_cmd, &g_ma_srv_rcv_sn, &pkt_len);
        if (pkt_len > DCL_BUFSIZE)
            return -1;
        if (pkt_len)
        {
            g_server_elmt[g_server_elmt_ma].recv_len = 0;
            g_server_elmt[g_server_elmt_ma].remain_len = pkt_len;
            g_ma_srv_rcv_phase = MA_SRV_RCV_DATA;
        }
        else
        {
            ma_cmd_parser(g_ma_srv_rcv_cmd, g_ma_srv_rcv_sn, NULL, 0);
            g_server_elmt[g_server_elmt_ma].recv_len = 0;
            g_server_elmt[g_server_elmt_ma].remain_len = sizeof(struct pkt_header);
            g_ma_srv_rcv_phase = MA_SRV_RCV_HEADER;
        }
    }
    else if (g_ma_srv_rcv_phase == MA_SRV_RCV_DATA)
    {
        ma_cmd_parser(g_ma_srv_rcv_cmd, g_ma_srv_rcv_sn, g_server_elmt[g_server_elmt_ma].recv_data,
                g_server_elmt[g_server_elmt_ma].recv_len);
        g_server_elmt[g_server_elmt_ma].recv_len = 0;
        g_server_elmt[g_server_elmt_ma].remain_len = sizeof(struct pkt_header);
        g_ma_srv_rcv_phase = MA_SRV_RCV_HEADER;
    }

    return 0;
}


static int ma_srv_send(int fd)
{
    int ret;

    if (g_counter_flag == THREAD_OPR_FLAG_REQ)
    {
        ret = ma_cmd_send_counter(fd);
        g_counter_flag = THREAD_OPR_FLAG_IDLE;
        if (ret < 0)
            return -1;
    }
    if (g_counter_struct_flag == THREAD_OPR_FLAG_REQ) {
        ret = ma_cmd_send_counter_struct(fd);
        g_counter_struct_flag = THREAD_OPR_FLAG_IDLE;
        if (ret < 0)
            return -1;
    }

    return 0;
}

void register_ma_server(void)
{
    if (ma_intf_conf.port_sguard == 0 || ma_intf_conf.port_ma == 0 || ma_intf_conf.sn == 0)
    {
        applog(LOG_ERR, APP_LOG_MASK_MA, "No connect to ma server. sguard port: %d, ma port: %d, sn: %d",
                ma_intf_conf.port_sguard, ma_intf_conf.port_ma, ma_intf_conf.sn);
        return ;
    }

    g_server_elmt[g_server_elmt_num].fd = 0;
    g_server_elmt[g_server_elmt_num].state = SOCK_FD_STATE_CLOSE;
    strcpy(g_server_elmt[g_server_elmt_num].ip, ma_intf_conf.ip_ma);
    g_server_elmt[g_server_elmt_num].port = ma_intf_conf.port_ma;
    g_server_elmt[g_server_elmt_num].retry_interval = 1000;
    g_server_elmt[g_server_elmt_num].counter = 0;
    g_server_elmt[g_server_elmt_num].flag = 0;
    g_server_elmt[g_server_elmt_num].recv_data = malloc(DCL_BUFSIZE);
    g_server_elmt[g_server_elmt_num].init = ma_srv_init;
    g_server_elmt[g_server_elmt_num].recv = ma_srv_recv;
    g_server_elmt[g_server_elmt_num].send = ma_srv_send;
    g_server_elmt_ma = g_server_elmt_num;
    g_server_elmt_num++;
}


void reload_ma_connect(void)
{
    g_server_elmt[g_server_elmt_sguard].flag = SOCK_SRV_FLAG_RECONN;
    g_server_elmt[g_server_elmt_sguard].port = ma_intf_conf.port_sguard;
    strcpy(g_server_elmt[g_server_elmt_sguard].ip, ma_intf_conf.ip_sguard);
    g_server_elmt[g_server_elmt_ma].flag = SOCK_SRV_FLAG_RECONN;
    g_server_elmt[g_server_elmt_ma].port = ma_intf_conf.port_ma;
    strcpy(g_server_elmt[g_server_elmt_ma].ip, ma_intf_conf.ip_ma);
    g_server_reconnect = THREAD_OPR_FLAG_REQ;
}
