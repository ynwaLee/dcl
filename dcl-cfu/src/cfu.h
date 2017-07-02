#ifndef __CFU_H__
#define __CFU_H__

#include <time.h>
#include <stdint.h>

#define APP_LOG_MASK_BASE   0x01
#define APP_LOG_MASK_CONF   0x02
#define APP_LOG_MASK_TELE   0x04
#define APP_LOG_MASK_FTP    0x08

#define DCL_STOP    (1 << 0)
#define DCL_KILL    (1 << 1)
#define DCL_DONE    (1 << 2)


extern time_t g_time;


extern uint8_t dcl_signal_flags;

#endif
