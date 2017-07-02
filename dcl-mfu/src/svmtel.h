#ifndef __SVMTEL_H__
#define __SVMTEL_H__

#include <stdint.h>
#include "mfusrv.h"

#define CALL_FLAG_VOICE  0
#define CALL_FLAG_FAX    1

typedef struct svmtel_req_s {
    unsigned long long callid;    //callid
    unsigned short scase;         // is case?
    unsigned short stime;         // is online?
    unsigned short callflag;      // is fax?
    unsigned short channel;       // 1 = up, 2 = down.
}svmtel_req_t;

void *svmtel_thread(void *arg);

#endif
