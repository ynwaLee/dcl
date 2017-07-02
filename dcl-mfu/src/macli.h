#ifndef __MACLI_H__
#define __MACLI_H__
#include "mfu.h"

#define CONF_RELOAD_NULL   (0x00)
#define CONF_RELOAD_DMS    (0x01)
#define CONF_RELOAD_MFU    (0x02)
#define CONF_RELOAD_ALL    (0x03)

typedef struct macli_para_s {
    unsigned int pid;    
    unsigned short sn;
    char *appname;
    ipport_t ma_addr;
    ipport_t sguard_addr;
}macli_para_t;

unsigned int get_reload_flag(void);
void set_reload_flag(unsigned int flag);

void *macli_thread(void *arg);

#endif
