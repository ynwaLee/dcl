#include "macli.h"
#include "mfa.h"
#include "sockop.h"
#include "common_header.h"

unsigned char g_counter_flag = THREAD_OPR_FLAG_IDLE;
static unsigned int g_reload_flag = 0;
unsigned int get_reload_flag(void)
{
    return g_reload_flag;
}
void set_reload_flag(unsigned int flag)
{
    g_reload_flag = flag;
}

static unsigned int g_counter_sn = 0;
static macli_para_t g_macli_para;
static unsigned char g_server_elmt_sguard;
static unsigned char g_server_elmt_ma;

static uint16_t get_status(void)
{
    uint16_t status;
    status = 1;
    return status;
}

static int sguard_srv_init(int fd)
{
    uint8_t pkt[MA_BUF_SIZE];
    uint32_t pkt_len;
    int ret;
    int sended = 0;
    macli_para_t *para = &g_macli_para;
    
    g_server_elmt[g_server_elmt_sguard].recv_len = 0;
    g_server_elmt[g_server_elmt_sguard].remain_len = 0;

    pkt_len = fill_regsguard_packet(pkt, para->pid, para->sn, 0, 0);
    ret = sock_send(fd, pkt, pkt_len, &sended);
    if (ret == -1) {
        return -1;
    }

    xapplog(LOG_INFO, APP_LOG_MASK_MA, "send register info %d bytes to sguard", pkt_len);
    return 0;
}

static int sguard_srv_recv(int fd)
{
    int ret, recved;
    uint8_t pkt[MA_BUF_SIZE];
    
    ret = sock_recv(fd, pkt, MA_BUF_SIZE, &recved);
    if (ret <= 0) {
        return -1;
    }

    return 0;
}

static int sguard_srv_send(int fd)
{
    static time_t sguard_last_keepalive = 0;
    int ret, sended;
    uint8_t pkt[MA_BUF_SIZE];
    uint32_t pkt_len;
    uint16_t status;
    time_t ntime;

    ntime = time(0);
    if (sguard_last_keepalive == ntime) {
        return 0;
    }

    status = get_status();
    pkt_len = fill_alive_packet(pkt, status);
    ret = sock_send(fd, pkt, pkt_len, &sended);
    if (ret == -1) {
        return -1;
    }

    sguard_last_keepalive = ntime;
    return 0;
}

static void register_sguard_server(void)
{
    macli_para_t *para = &g_macli_para;
    if (para->sguard_addr.port == 0 || para->ma_addr.port == 0 || para->sn == 0)
    {
        applog(LOG_ERR, APP_LOG_MASK_MA, "No connect to sguard server. sguard port: %d, ma port: %d, sn: %d", para->sguard_addr.port, para->ma_addr.port, para->sn);
        return ;
    }

    g_server_elmt[g_server_elmt_num].fd = 0;
    g_server_elmt[g_server_elmt_num].state = SOCK_FD_STATE_CLOSE;
    strcpy(g_server_elmt[g_server_elmt_num].ip, para->sguard_addr.ip);
    g_server_elmt[g_server_elmt_num].port = para->sguard_addr.port;
    g_server_elmt[g_server_elmt_num].retry_interval = 100;
    g_server_elmt[g_server_elmt_num].counter = 0;
    g_server_elmt[g_server_elmt_num].flag = 0;
    g_server_elmt[g_server_elmt_num].recv_data = NULL;
    g_server_elmt[g_server_elmt_num].init = sguard_srv_init;
    g_server_elmt[g_server_elmt_num].recv = sguard_srv_recv;
    g_server_elmt[g_server_elmt_num].send = sguard_srv_send;
    g_server_elmt_sguard = g_server_elmt_num;
    g_server_elmt_num++;
}

static int ma_cmd_reload(uint8_t *data, uint32_t data_len)
{
    unsigned char flag = 0;
    char *conf[5];
    uint8_t num, i;

    num = get_ma_reload_conf(data, data_len, &conf[0], 5);
    for (i = 0; i < num; i++) {
        xapplog(LOG_INFO, APP_LOG_MASK_MA, "reload conf %u files: %s", i, conf[i]);
        if (strcmp(conf[i], DMS_CONF_FILE) == 0) {
            flag |= CONF_RELOAD_DMS;
        }
        else if (strcmp(conf[i], MFA_CONF_FILE) == 0) {
            flag |= CONF_RELOAD_MFA;
        }
        else if (strcmp(conf[i], "all") == 0) {
            flag = CONF_RELOAD_ALL;
        }else {
            xapplog(LOG_WARNING, APP_LOG_MASK_MA, "can't recognise the filename: %s recved from ma.", conf[i]);
            return 0;
        }
    }

    g_reload_flag = flag;

    return 0;
}

static int ma_cmd_send_counter(int sockfd)
{
    int ret;
    int sended;
    uint8_t pkt_buf[1500];
    uint16_t pkt_len;
    int mfa_counter_cmd = 20;

    mfa_counter_t counter = {0};
    int counterlen = sizeof(mfa_counter_t);
    bcopy(&g_mfa_counter, &counter, counterlen);

    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "request(total: %llu, valid: %llu, invalid: %llu), response(total: %llu, valid: %llu, invalid: %llu, voc frames: %llu)", 
    counter.req_total, 
    counter.req_valid, 
    counter.req_invalid, 
    counter.res_total, 
    counter.res_valid, 
    counter.res_invalid,
    counter.voc_frame);

    counter.req_total = htobe64(counter.req_total);
    counter.req_valid = htobe64(counter.req_valid);
    counter.req_invalid = htobe64(counter.req_invalid);
    counter.res_total = htobe64(counter.res_total);
    counter.res_valid = htobe64(counter.res_valid);
    counter.res_invalid = htobe64(counter.res_invalid);
    counter.voc_frame = htobe64(counter.voc_frame);

    pkt_len = fill_ma_counter2(pkt_buf, mfa_counter_cmd, g_counter_sn, (uint8_t*)&counter, counterlen);
    
    ret = sock_send(sockfd, pkt_buf, pkt_len, &sended);
    if (ret == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_MA, "send counter data less than pkt_len %d bytes", pkt_len);
        return -1;
    }
    return 0;
}

static int ma_cmd_parser(uint16_t cmd, uint16_t sn, uint8_t *pkt, uint16_t pkt_len)
{
    unsigned int v;
    int ret;
    uint8_t pkt_buf[2048];

    switch(cmd) {
        case COUNTER_STRUCT:
            xapplog(LOG_INFO, APP_LOG_MASK_MA, "get counter struct cmd from ma, sn: %d.", sn);
            ret = fill_ma_count_info_struct(pkt_buf, sn, "mfa", "mfa", g_mfa_count_struct_num, g_mfa_count_struct);
            sock_send_buf(g_server_elmt[g_server_elmt_ma].fd, pkt_buf, ret);
            break;
        case RELOAD:
            xapplog(LOG_INFO, APP_LOG_MASK_MA, "get reload config file cmd from ma");
            ma_cmd_reload(pkt, pkt_len);
            break;
        case GET_COUNTER:
            xapplog(LOG_INFO, APP_LOG_MASK_MA, "get counter cmd from ma");
            g_counter_sn = sn;
            g_counter_flag = THREAD_OPR_FLAG_OK;
            break;
        case DEBUG_ON:
            xapplog(LOG_INFO, APP_LOG_MASK_MA, "get debug_on cmd from ma");
            applog_debug_on(&(g_server_elmt[g_server_elmt_ma].fd));
            break;
        case DEBUG_OFF:
            xapplog(LOG_INFO, APP_LOG_MASK_MA, "get debug_off cmd from ma");
            applog_debug_off();
            break;
        case SYSLOG_LEVEL:
            xapplog(LOG_INFO, APP_LOG_MASK_MA, "get syslog_level cmd from ma");
            ma_cmd_get_loglevel(pkt, pkt_len, &v);
            applog_set_log_level(v);
            break;
        case DEBUG_MASK:
            xapplog(LOG_INFO, APP_LOG_MASK_MA, "get debug_mask cmd from ma");
            ma_cmd_get_logmask(pkt, pkt_len, &v);
            applog_set_debug_mask(v);
            break;
        case SHOW_SYS_LEV:
            xapplog(LOG_INFO, APP_LOG_MASK_MA, "get show_sys_lev cmd from ma");
            ret = fill_loglevel_packet(pkt_buf, sn, g_app_log_level);
            sock_send_buf(g_server_elmt[g_server_elmt_ma].fd, pkt_buf, ret);
            break;
        case SHOW_DEBUG_MASK:
            xapplog(LOG_INFO, APP_LOG_MASK_MA, "get show_debug_mask cmd from ma");
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
    uint8_t pkt[MA_BUF_SIZE];
    uint32_t pkt_len;
    int ret;
    int sended;

    macli_para_t *para = &g_macli_para;
    g_server_elmt[g_server_elmt_ma].recv_len = 0;
    g_server_elmt[g_server_elmt_ma].remain_len = sizeof(struct pkt_header);
    g_ma_srv_rcv_phase = MA_SRV_RCV_HEADER;

    pkt_len = fill_regma_packet(pkt, para->pid, para->sn);
    ret = sock_send(fd, pkt, pkt_len, &sended);
    if (ret == -1) {
        return -1;
    }
    xapplog(LOG_INFO, APP_LOG_MASK_MA, "send regist info %d bytes to ma", pkt_len);
    return 0;
}

static int ma_srv_recv(int fd)
{
    uint16_t pkt_len;

    if (g_ma_srv_rcv_phase == MA_SRV_RCV_HEADER)
    {
        ma_header_get_cmd_sn_len(g_server_elmt[g_server_elmt_ma].recv_data, &g_ma_srv_rcv_cmd, &g_ma_srv_rcv_sn, &pkt_len);
        if (pkt_len > MA_BUF_SIZE)
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

    if (g_counter_flag == THREAD_OPR_FLAG_OK)
    {
        ret = ma_cmd_send_counter(fd);
        g_counter_flag = THREAD_OPR_FLAG_IDLE;
        if (ret < 0)
            return -1;
    }
    return 0;
}

static void register_ma_server(void)
{
    macli_para_t *para = &g_macli_para;
    if (para->sguard_addr.port == 0 || para->ma_addr.port == 0 || para->sn == 0)
    {
        applog(LOG_ERR, APP_LOG_MASK_MA, "No connect to sguard server. sguard port: %d, ma port: %d, sn: %d", para->sguard_addr.port, para->ma_addr.port, para->sn);
        return ;
    }

    g_server_elmt[g_server_elmt_num].fd = 0;
    g_server_elmt[g_server_elmt_num].state = SOCK_FD_STATE_CLOSE;
    strcpy(g_server_elmt[g_server_elmt_num].ip, para->ma_addr.ip);
    g_server_elmt[g_server_elmt_num].port = para->ma_addr.port;
    g_server_elmt[g_server_elmt_num].retry_interval = 100;
    g_server_elmt[g_server_elmt_num].counter = 0;
    g_server_elmt[g_server_elmt_num].flag = 0;
    g_server_elmt[g_server_elmt_num].recv_data = malloc(MA_BUF_SIZE);
    g_server_elmt[g_server_elmt_num].init = ma_srv_init;
    g_server_elmt[g_server_elmt_num].recv = ma_srv_recv;
    g_server_elmt[g_server_elmt_num].send = ma_srv_send;
    g_server_elmt_ma = g_server_elmt_num;
    g_server_elmt_num++;
}


static void reload_ma_connect(void)
{
    macli_para_t *para = &g_macli_para;

    g_server_elmt[g_server_elmt_sguard].flag = SOCK_SRV_FLAG_RECONN;
    g_server_elmt[g_server_elmt_sguard].port = para->sguard_addr.port;
    strcpy(g_server_elmt[g_server_elmt_sguard].ip, para->sguard_addr.ip);

    g_server_elmt[g_server_elmt_ma].flag = SOCK_SRV_FLAG_RECONN;
    g_server_elmt[g_server_elmt_ma].port = para->ma_addr.port;
    strcpy(g_server_elmt[g_server_elmt_ma].ip, para->ma_addr.ip);
    g_server_reconnect = THREAD_OPR_FLAG_REQ;
}

static int get_dms_conf(macli_para_t *maclipara)
{
    char dump_config = 0;
    char *value = NULL;

    maclipara->sguard_addr.port = 2000;
    maclipara->ma_addr.port = 2001;

    char dms_conf_path[DMS_CONF_PATH_LEN] = {0};
    snprintf(dms_conf_path, DMS_CONF_PATH_LEN, "%s/%s", DMS_CONF_DIR, DMS_CONF_FILE);

    ConfInit();
    if (ConfYamlLoadFile(dms_conf_path) != 0) {
        exit(EXIT_FAILURE);
    }

    if (dump_config) {
        applog(LOG_INFO, APP_LOG_MASK_CDR, "Dump all config variable:");
        ConfDump();
    }

    if (ConfGet("ma.sguardport", &value) == 1) {
        maclipara->sguard_addr.port = atoi(value);
    }

    if (ConfGet("ma.maport", &value) == 1) {
        maclipara->ma_addr.port = atoi(value);
    }

    ConfDeInit();

    return 0;
}

static int macli_para_init(macli_para_t *maclipara, cmdline_para_t *cmdlpara)
{
    int ret = 0;
    unsigned int vint;

    maclipara->appname = cmdlpara->argv[0];
    maclipara->pid = getpid();
    
    ret = get_arg_value_uint(cmdlpara->argc, cmdlpara->argv, "--sn", &vint);
    if (ret == 0) {
        maclipara->sn = vint;
    }else {
        applog(LOG_INFO, APP_LOG_MASK_CDR, "mfa's sn is not set or error, won't connect to ma.");
    }

    snprintf(maclipara->ma_addr.ip, 16, "%s", "127.0.0.1");
    snprintf(maclipara->sguard_addr.ip, 16, "%s", "127.0.0.1");

    mfa_mutex_lock();
    get_dms_conf(maclipara);
    mfa_mutex_unlock();

    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "macli paras, pid: %u, sn: %u, appname: %s, ma addr(ip: %s, port: %u), sguard addr(ip: %s, port: %u)", maclipara->pid, maclipara->sn, maclipara->appname, maclipara->ma_addr.ip, maclipara->ma_addr.port, maclipara->sguard_addr.ip, maclipara->sguard_addr.port);

    return 0;
}

void *macli_thread(void *arg)
{
    cmdline_para_t *cmdlpara = (cmdline_para_t *)arg;
    macli_para_t *maclipara = &g_macli_para;
    
    int done = false;
    int macli_para_init_flag = false;
    int macli_register_flag = false;
    int macli_running_flag = false;
    while (!done) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }

        if (!macli_para_init_flag) {
            macli_para_init(maclipara, cmdlpara);
            macli_para_init_flag = true;
        }

        if (!macli_register_flag) {
            register_ma_server();
            register_sguard_server();
            macli_register_flag = true;
        }
        
        if (!macli_running_flag) {
            pthread_t tid;
            if (pthread_create(&tid, NULL, sock_clients_proc, NULL) == -1) {
                xapplog(LOG_ERR, APP_LOG_MASK_BASE, "create sock clients proc thread error.\n");
            }
            macli_running_flag = true;
        }

        if (g_reload_flag & CONF_RELOAD_DMS) {
            macli_para_init(maclipara, cmdlpara);
            reload_ma_connect();
            g_reload_flag &= ~CONF_RELOAD_DMS;
        }
        
        usleep(1000);
    }
    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "ma client thread exit successful.\n");
    return NULL;
}
