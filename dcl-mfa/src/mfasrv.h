#ifndef __MFASRV_H__
#define __MFASRV_H__
#include "mfa.h"
#include "onlinelist.h"
#include <stdint.h>

typedef struct path_s {
    int id;
    char path[MFA_PATH_LEN];
    struct path_s *next;
}path_t;

typedef struct mfasrv_para_s {
    ipport_t *svmmobladdr;
    ipport_t *voipAaddr;
    ipport_t *voipBaddr;
    path_t *pathmoblhdr;
    path_t *pathvoipAhdr;
    path_t *pathvoipBhdr;
    
}mfasrv_para_t;

int mfasrv_para_init(mfasrv_para_t *mfasrvpara, cmdline_para_t *cmdlpara);
inline mfasrv_para_t *mfasrv_para_get(void);
inline online_list_t *online_list_get(void);
extern unsigned long long g_mfasrv_reload_time;

void *mfasrv_thread(void *arg);

#endif
