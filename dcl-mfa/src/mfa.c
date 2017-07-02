#include "mfa.h"
#include "common_header.h"
#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>
#include <pthread.h>

int g_mfa_count_struct_num = 7;
#define STR_REQ_TOTAL   "req_total"
#define STR_REQ_VALID   "req_valid"
#define STR_REQ_INVALID "req_invalid"
#define STR_RES_TOTAL   "res_total"
#define STR_RES_VALID   "res_valid"
#define STR_RES_INVALID "res_invalid"
#define STR_VOC_FRAME   "voc_frame"
struct count_struct g_mfa_count_struct[] = {
    { STR_REQ_TOTAL,   8},
    { STR_REQ_VALID,   8},
    { STR_REQ_INVALID, 8},
    { STR_RES_TOTAL,   8},
    { STR_RES_VALID,   8},
    { STR_RES_INVALID, 8},
    { STR_VOC_FRAME,   8},
};

pthread_mutex_t g_mfa_mutex;
inline void mfa_mutex_init(void)
{
    pthread_mutex_init(&g_mfa_mutex, NULL);
}
inline void mfa_mutex_lock(void)
{
    pthread_mutex_lock(&g_mfa_mutex);
}
inline void mfa_mutex_unlock(void)
{
    pthread_mutex_unlock(&g_mfa_mutex);
}

mfa_counter_t g_mfa_counter;

void init_log_para(void)
{
    char filename[MFA_CONF_PATH_LEN] = {0};
    snprintf(filename, MFA_CONF_PATH_LEN, "%s/%s", MFA_CONF_DIR, MFA_CONF_FILE);
    ConfInit();
    if (ConfYamlLoadFile(filename) != 0) {
        /* Error already displayed. */
        exit(EXIT_FAILURE);
    }
    get_log_para();
    ConfDeInit();
}


void mfa_applog(char *buff, const char *pfile, const char *pfunc, int line, const char *fmt, ...)
{
    va_list ap;
    int len = snprintf(buff, MAX_LOG_LEN, "%s:%d   ", pfile, line);
    va_start(ap, fmt);
    vsnprintf(buff + len - 1, MAX_LOG_LEN - len, fmt, ap);
    va_end(ap);
}
