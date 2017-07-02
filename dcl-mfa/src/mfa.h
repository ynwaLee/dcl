#ifndef __MFA_H__
#define __MFA_H__
#include "../../common/ma_pkt.h"

#define MFA_CONF_PATH_LEN  1024
#define MFA_CONF_DIR "/usr/local/etc/mfa"
#define MFA_CONF_FILE "mfa.yaml"

#define DMS_CONF_PATH_LEN  1024
#define DMS_CONF_DIR       "/usr/local/etc"
#define DMS_CONF_FILE      "dms.yaml"

#define MA_BUF_SIZE  4096
#define MAX_LOG_LEN  4096
#define MFA_PATH_LEN 4096

typedef struct svmmob_req_s {
    int systypes;
    
    unsigned char type;
    unsigned short len;
    unsigned int online;
    unsigned long long mntid;
    unsigned int channel;
    char vocfile[1024];
}svmmob_req_t;

typedef struct ipport_s {
    int id;
    char ip[16];
    unsigned short port;
    struct ipport_s *next;
}ipport_t;

typedef struct cmdline_para_s {
    int argc;
    char **argv;
}cmdline_para_t;

typedef struct mfa_counter_s {
    unsigned long long req_total;
    unsigned long long req_valid;
    unsigned long long req_invalid;
    unsigned long long res_total;
    unsigned long long res_valid;
    unsigned long long res_invalid;
    unsigned long long voc_frame;
}mfa_counter_t;

extern mfa_counter_t g_mfa_counter;
extern int g_mfa_count_struct_num;
extern struct count_struct g_mfa_count_struct[];


typedef struct _ipportsockfd
{
    int socketfd;
    int systypes;
    char ip[16];
    unsigned short port;
}ipportsockfd;
void mfa_applog(char *buff, const char *pfile, const char *pfunc, int line, const char *fmt, ...);
#define xapplog(level, mask, ...) do{      \
        char buff[MAX_LOG_LEN] = {0};      \
        const char *pfile = __FILE__;            \
        const char *pfunc = __func__;            \
        int  line = __LINE__;              \
        mfa_applog(buff, pfile, pfunc, line, __VA_ARGS__);     \
        applog(level, mask, "%s", buff);   \
        printf("%s\n", buff);              \
}while (0)

extern void init_log_para();

extern void mfa_mutex_init(void);
extern void mfa_mutex_lock(void);
extern void mfa_mutex_unlock(void);
#if 1
static int encrypt_wave_data(unsigned char *data, unsigned int data_len)
{
    unsigned int i;

    for (i = 0; i < data_len; i++) {
        data[i] = data[i] ^ 0x53;
    }

    return 0;
}
#endif
#endif
