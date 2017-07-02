
#ifndef __MA_PKT_H__ 
#define __MA_PKT_H__ 

#include <stdint.h>

#define RELOAD          23
#define GET_COUNTER     19
#define DEV_SOFT_STATUS 14
#define SWITCH_DEVICE   13
#define DEBUG_ON        30
#define DEBUG_OFF        31
#define SYSLOG_LEVEL    32
#define SHOW_SYS_LEV    33
#define DEBUG_MASK        34
#define SHOW_DEBUG_MASK    35
#define COUNTER_STRUCT 39


extern uint32_t debug_sd_flg;
extern uint16_t ma_num;

struct pkt_header {
    uint8_t ver;
    uint8_t reply;
    uint16_t cmd;
    uint16_t num;
    uint16_t len;
};


typedef struct DEV_SOFT_STATUS_ST_
{
    uint32_t id;
    uint32_t status;
    char dev[32];
    char soft[32];
}DEV_SOFT_STATUS_ST;

uint16_t fill_info(uint8_t *pkt, uint8_t *info, uint16_t len, uint16_t type);
uint32_t fill_regsguard_packet(uint8_t *pkt, uint32_t pid, uint32_t sn, char *pc_ip, int i_sid);
uint32_t fill_alive_packet(uint8_t *pkt, uint16_t status);
uint32_t fill_regma_packet(uint8_t *pkt, uint32_t pid, uint32_t sn);
uint32_t fill_ma_counter(uint8_t *pkt, uint16_t type,
        uint8_t *cntr, uint16_t len);
uint32_t fill_ma_counter2(uint8_t *pkt, uint16_t type, uint16_t sn,
        uint8_t *cntr, uint16_t len);
uint8_t get_ma_reload_conf(uint8_t *data, uint32_t data_len,
        char **conf, uint32_t num);
uint8_t ma_cmd_get_dev_soft_status(uint8_t *data, uint32_t data_len,
        char* sf_name, uint8_t nlen, DEV_SOFT_STATUS_ST *dss, uint8_t num);
int ma_cmd_get_data(uint8_t *pkt, uint32_t pkt_len,
        uint16_t *cmd, uint8_t **data, uint32_t *data_len);
void ma_header_get_cmd_len(uint8_t *pkt, uint16_t *cmd, uint16_t *data_len);
void ma_header_get_cmd_sn_len(uint8_t *pkt, uint16_t *cmd, uint16_t *sn, uint16_t *data_len);

int ma_cmd_get_master_slave_status(unsigned char *puc_data,int i_data_len,
    char *pc_sname,int i_sname_len, unsigned int *pi_ms_status, unsigned int *pi_sid,
    char *pc_devname,int i_devname_len,char *pc_ip,int i_ip_len);

extern void send_to_ma_log(int level, const char *fmt, ...);
int ma_cmd_get_loglevel(void *pv_data, unsigned int ui_data_len, unsigned int *pi_ret);
int fill_loglevel_packet(void *pv_data, unsigned short us_cmd_num, unsigned int i_loglevel);
int ma_cmd_get_logmask(void *pv_data, unsigned int ui_data_len, unsigned int *pi_ret);
int fill_logmask_packet(void *pv_data, unsigned short us_cmd_num, unsigned int i_log_mask);
extern void (*gpf_ma_log_out)(char *);

struct count_struct
{
    char name[32];
    unsigned int len;
};

extern int fill_ma_count_info_struct(unsigned char *pkt, unsigned short pkt_head_num, char *soft_name, char *proc_name, unsigned int filed_num, struct count_struct *filed_array);


#endif

