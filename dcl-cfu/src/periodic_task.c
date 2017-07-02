#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "cfu.h"
#include "conf_parser.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"

void *thread_periodic_task(void *arg)
{
    int current_day;
    int last_day;
    time_t nextday_time;
    struct tm nextday;
    struct tm today;

    last_day = 0;
    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }
        localtime_r(&g_time, &today);
        current_day = today.tm_mday;

        if (current_day != last_day && last_day != 0) {
            cleanup_backup_dirs(cfu_conf.local_backup_dir);

            nextday_time = g_time + 24 * 60 * 60;
            localtime_r(&nextday_time, &nextday);
            create_backup_dirs(cfu_conf.local_backup_dir, nextday.tm_year + 1900, nextday.tm_mon + 1, nextday.tm_mday);
        }

        last_day = current_day;

        sleep(1);
    }
    
    pthread_exit(NULL);
}

