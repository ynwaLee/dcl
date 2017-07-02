
#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "../../common/applog.h"
#define APP_LOG_MASK_RFU APP_LOG_MASK_BASE

#define CONFIG_DIR  "/usr/local/etc/rfu/"  // "/home/edu/job/dcl/rfu/etc/"
#define CONFIG_RFU "rfu.yaml"

#define RESTORE_SYS_SVM 0x01 
#define RESTORE_SYS_WIRELESS 0x02
#define RESTORE_SYS_VOIP_A 0x04
#define RESTORE_SYS_VOIP_B 0x08

#define MAX_VOIPA_NUM 64

struct yj_sys_conf
{
    char ac_server_ip[16];
    char ac_pwd[255];
    char ac_operator_list[100];
};

struct conf
{
    //path
    char ac_svm_conf_path[255];
    char ac_conf_path_base[255];

    //das
    int das_signal_port;
    int das_data_port;
    char ac_das_ip[16];
    
    //svm
    char ac_svm_ser_ip[16];
    
    struct yj_sys_conf wireless;
    struct yj_sys_conf voip_a[MAX_VOIPA_NUM];
    struct yj_sys_conf voip_b;
	int voipa_count;
    
    //ma
    int if_link_ma_flg;
    int ma_port;
    int sguard_port;

    //cfu
    int cfu_listen_port;

    //restore system
    volatile int restore_sys_flg;
};

extern struct conf gst_conf;
extern int gi_das_data_fd;
extern int gi_sn;
extern char gac_config_path[500];


#define MY_CLOSE(macro_fd) \
({\
    if( macro_fd>0 )\
    {\
        close(macro_fd);\
    }\
    macro_fd = -1;\
})

#define MY_MALLOC(macro_size) \
({\
    void *pv_macro_var = NULL;\
    if( macro_size>0 )\
    {\
        if( (pv_macro_var=malloc(macro_size))==NULL )\
        {\
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "in the %s function, call malloc function failed, exit proccess!", __FUNCTION__);\
            exit(-1);\
        }\
        memset (pv_macro_var, 0, macro_size);\
    }\
    pv_macro_var;\
})

#define MY_REALLOC(pc_macro_src,macro_size) \
({\
    void *pv_macro_var = NULL;\
    if( macro_size>0 && pc_macro_src!=NULL)\
    {\
        if( (pv_macro_var=realloc(pc_macro_src, macro_size))==NULL )\
        {\
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "in the %s function, call realloc function failed, exit proccess!", __FUNCTION__);\
            exit(-1);\
        }\
    }\
    pv_macro_var;\
})

#define MY_FREE(pv_macro_src) \
({\
    if( pv_macro_src!=NULL )\
    {\
        free (pv_macro_src);\
        pv_macro_src = NULL;\
    }\
 })


#endif


