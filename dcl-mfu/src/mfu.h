#ifndef __MFU_H__
#define __MFU_H__

#include "../../common/ma_pkt.h"
#define MFU_CONF_PATH_LEN  1024
#define MFU_CONF_DIR "/usr/local/etc/mfu"
#define MFU_CONF_FILE "mfu.yaml"

#define DMS_CONF_PATH_LEN  1024
#define DMS_CONF_DIR       "/usr/local/etc"
#define DMS_CONF_FILE      "dms.yaml"

#define MA_BUF_SIZE  4096
#define MAX_LOG_LEN  4096
#define MFA_PATH_LEN 4096


typedef struct ipport_s {
    char ip[16];
    unsigned short port;
	char city_code[16];
    struct ipport_s *next;
}ipport_t;

typedef struct cmdline_para_s {
    int argc;
    char **argv;
}cmdline_para_t;

typedef struct mfu_counter_s {

    unsigned long long media_req_total;
    unsigned long long media_req_valid;
    unsigned long long media_req_invalid;
    unsigned long long media_res_total;
    unsigned long long media_res_valid;
    unsigned long long media_res_invalid;

    unsigned long long svmtel_req_total;
    unsigned long long svmtel_req_valid;
    unsigned long long svmtel_req_invalid;
    unsigned long long svmtel_res_total;
    unsigned long long svmtel_res_valid;
    unsigned long long svmtel_res_invalid;

    unsigned long long svmmob_req_total;
    unsigned long long svmmob_req_valid;
    unsigned long long svmmob_req_invalid;
    unsigned long long svmmob_res_total;
    unsigned long long svmmob_res_valid;
    unsigned long long svmmob_res_invalid;

    //shiwb
    
    unsigned long long svmvoipA_req_total;
    unsigned long long svmvoipA_req_valid;
    unsigned long long svmvoipA_req_invalid;
    unsigned long long svmvoipA_res_total;
    unsigned long long svmvoipA_res_valid;
    unsigned long long svmvoipA_res_invalid;

    
    unsigned long long svmvoipB_req_total;
    unsigned long long svmvoipB_req_valid;
    unsigned long long svmvoipB_req_invalid;
    unsigned long long svmvoipB_res_total;
    unsigned long long svmvoipB_res_valid;
    unsigned long long svmvoipB_res_invalid;
}mfu_counter_t;


extern mfu_counter_t g_mfu_counter;
extern int g_mfu_count_struct_num;
extern struct count_struct g_mfu_count_struct[];

void mfu_applog(char *buff, const char *pfile, const char *pfunc, int line, const char *fmt, ...);
#define xapplog(level, mask, ...) do{      \
        char buff[MAX_LOG_LEN] = {0};      \
        const char *pfile = __FILE__;            \
        const char *pfunc = __func__;            \
        int  line = __LINE__;              \
        mfu_applog(buff, pfile, pfunc, line, __VA_ARGS__);     \
        applog(level, mask, "%s", buff);   \
        printf("%s\n", buff);              \
}while (0)

extern void init_log_para();

extern void mfu_mutex_init(void);
extern void mfu_mutex_lock(void);
extern void mfu_mutex_unlock(void);

#endif
