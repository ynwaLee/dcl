#include "mfu.h"
#include "common_header.h"
#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>
#include <pthread.h>

int g_mfu_count_struct_num = 18;
#define STR_MEDIA_REQ_TOTAL   "media_req_total"
#define STR_MEDIA_REQ_VALID   "media_req_valid"
#define STR_MEDIA_REQ_INVALID "media_req_invalid"
#define STR_MEDIA_RES_TOTAL   "media_res_total"
#define STR_MEDIA_RES_VALID   "media_res_valid"
#define STR_MEDIA_RES_INVALID "media_res_invalid"

#define STR_SVMTEL_REQ_TOTAL   "svmtel_req_total"
#define STR_SVMTEL_REQ_VALID   "svmtel_req_valid"
#define STR_SVMTEL_REQ_INVALID "svmtel_req_invalid"
#define STR_SVMTEL_RES_TOTAL   "svmtel_res_total"
#define STR_SVMTEL_RES_VALID   "svmtel_res_valid"
#define STR_SVMTEL_RES_INVALID "svmtel_res_invalid"

#define STR_SVMMOB_REQ_TOTAL   "svmmob_req_total"
#define STR_SVMMOB_REQ_VALID   "svmmob_req_valid"
#define STR_SVMMOB_REQ_INVALID "svmmob_req_invalid"
#define STR_SVMMOB_RES_TOTAL   "svmmob_res_total"
#define STR_SVMMOB_RES_VALID   "svmmob_res_valid"
#define STR_SVMMOB_RES_INVALID "svmmob_res_invalid"
struct count_struct g_mfu_count_struct[] = {
    {STR_MEDIA_REQ_TOTAL, 8},
    {STR_MEDIA_REQ_VALID, 8},  
    {STR_MEDIA_REQ_INVALID, 8},
    {STR_MEDIA_RES_TOTAL, 8},
    {STR_MEDIA_RES_VALID, 8},
    {STR_MEDIA_RES_INVALID, 8},

    {STR_SVMTEL_REQ_TOTAL, 8},
    {STR_SVMTEL_REQ_VALID, 8},
    {STR_SVMTEL_REQ_INVALID, 8},
    {STR_SVMTEL_RES_TOTAL, 8},
    {STR_SVMTEL_RES_VALID, 8},
    {STR_SVMTEL_RES_INVALID, 8},

    {STR_SVMMOB_REQ_TOTAL, 8},
    {STR_SVMMOB_REQ_VALID, 8},
    {STR_SVMMOB_REQ_INVALID, 8},
    {STR_SVMMOB_RES_TOTAL, 8},
    {STR_SVMMOB_RES_VALID, 8},
    {STR_SVMMOB_RES_INVALID, 8}
};

pthread_mutex_t g_mfu_mutex;
inline void mfu_mutex_init(void)
{
    pthread_mutex_init(&g_mfu_mutex, NULL);
}
inline void mfu_mutex_lock(void)
{
    pthread_mutex_lock(&g_mfu_mutex);
}
inline void mfu_mutex_unlock(void)
{
    pthread_mutex_unlock(&g_mfu_mutex);
}

mfu_counter_t g_mfu_counter;

void init_log_para(void)
{
    char filename[MFU_CONF_PATH_LEN] = {0};
    snprintf(filename, MFU_CONF_PATH_LEN, "%s/%s", MFU_CONF_DIR, MFU_CONF_FILE);
    ConfInit();
    if (ConfYamlLoadFile(filename) != 0) {
        /* Error already displayed. */
        exit(EXIT_FAILURE);
    }
    get_log_para();
    ConfDeInit();
}


void mfu_applog(char *buff, const char *pfile, const char *pfunc, int line, const char *fmt, ...)
{
    va_list ap;
    int len = snprintf(buff, MAX_LOG_LEN, "%s:%-5d", pfile, line);
    va_start(ap, fmt);
    vsnprintf(buff + len - 1, MAX_LOG_LEN - len, fmt, ap);
    va_end(ap);
}
