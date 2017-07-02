#ifndef __LISPR_H__
#define __LISPR_H__

#define LISPR_RETVAL_ERROR_DATA    0
#define LISPR_RETVAL_ERROR_UPDATE  1
#define LISPR_RETVAL_ERROR_NONE    2
#define LISPR_RETVAL_ERROR_CONN    3
extern void *thread_lispr(void *argv);

#endif
