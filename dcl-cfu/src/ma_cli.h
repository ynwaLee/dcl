#ifndef __MA_CLI_H__
#define __MA_CLI_H__
#include "atomic.h"

#define CONF_RELOAD_NULL    0
#define CONF_RELOAD_CFU        1
#define CONF_RELOAD_DMS        2
#define CONF_RELOAD_ALL        0x3

#define DEVICE_STATUS_SLAVER      0 
#define DEVICE_STATUS_MASTER       1
#define DEVICE_STATUS_WAITING     2
#define DEVICE_STATUS_SINGLE      4
#define DEVICE_STATUS_SWITCHING2M 8   //switch to master 
#define DEVICE_STATUS_SWITCHING2S 16  //switch to slaver

#define MA_SERV_IP         "127.0.0.1"

typedef struct MA_INTF_CONF
{
    uint32_t pid;
    uint16_t sn;
    uint16_t port_sguard;
    uint16_t port_ma;
    char ip_sguard[16];
    char ip_ma[16];
    char *app_name;
}MA_INTF_CONF;

typedef struct STRUCT_PKT_COUNTER_
{
    uint64_t svm_file_num;
    uint64_t svm_call_num;
    uint64_t svm_mass_file_num;
    uint64_t svm_mass_call_num;
    uint64_t svm_case_file_num;
    uint64_t svm_case_call_num;
    uint64_t mobile_file_num;
    uint64_t mobile_call_num;
    uint64_t mobile_mass_file_num;
    uint64_t mobile_mass_call_num;
    uint64_t mobile_case_file_num;
    uint64_t mobile_case_call_num;
    uint64_t das_file_num;
    uint64_t das_call_num;
    uint64_t das_mass_file_num;
    uint64_t das_mass_call_num;
    uint64_t das_case_file_num;
    uint64_t das_case_call_num;
    uint64_t das_upload_success;

}STRUCT_PKT_COUNTER;

void register_sguard_server(void);
void register_ma_server(void);
void reload_ma_connect(void);


#endif
