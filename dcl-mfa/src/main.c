#include "common_header.h"
#include "mfa.h"
#include "mfasrv.h"
#include "macli.h"
#include <pthread.h>
#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>

unsigned char g_daemon_flag = 1;
int main(int argc, char *argv[])
{
    if(argc == 2 && (strcmp(argv[1], "-v") == 0))
    {
        printf("version: %s\n", VERSION);
        exit(EXIT_SUCCESS);
    }
    
    if (g_daemon_flag) {
        daemonize();
    }
    open_applog("mfa", LOG_NDELAY, DEF_APP_SYSLOG_FACILITY);
    applog_set_debug_mask(0);

    init_log_para();
    init_signal_catch();
    mfa_mutex_init();

    pthread_t mfasrv_tid;
    pthread_t macli_tid;
    cmdline_para_t *para = alloca(sizeof(cmdline_para_t));
    para->argc = argc;
    para->argv = argv;

#if 1
    if (pthread_create(&mfasrv_tid, NULL, mfasrv_thread, (void*)para) == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "%s", "Failed to create thread mfasrv, proccess exit.");
        exit(EXIT_FAILURE);
    }
#endif

#if 1
    if (pthread_create(&macli_tid, NULL, macli_thread, (void *)para) == -1) {
        xapplog(LOG_ERR, APP_LOG_MASK_BASE, "%s", "Failed to create thread macli, process exit");
        exit(EXIT_FAILURE);
    }
#endif
    int done = 0;
    while (!done) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }

        if (get_reload_flag()) {
            mfa_mutex_lock();
            init_log_para();
            mfa_mutex_unlock();
        }

        usleep(1000);
    }

    pthread_join(mfasrv_tid, NULL);
    pthread_join(macli_tid, NULL);

    xapplog(LOG_INFO, APP_LOG_MASK_BASE, "%s", "Mfa is done.");
    return 0;
}
