#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include "cfu.h"
#include "svm_cdr.h"
#include "mobile_cdr.h"
#include "voipa_cdr.h"
#include "voipb_cdr.h"
#include "conf_parser.h"
#include "ma_cli.h"
#include "atomic.h"
#include "file_fifo.h"
#include "mass_ftp_upload.h"
#include "case_ftp_upload.h"
#include "zip_ftp_upload.h"
#include "rfu_client.h"
#include "svm_online.h"
#include "dcl_online.h"
#include "mobile_online.h"
#include "voipa_online.h"
#include "voipb_online.h"
#include "counter.h"
#include "periodic_task.h"
#include "lispr.h"
#include "mobmass_fifo.h"
#include "mobonline_array.h"

#include "../../common/conf.h"
#include "../../common/applog.h"
#include "../../common/sock_clis.h"
#include "../../common/daemon.h"
#include "../../common/thread_flag.h"
#include "../../common/signal_catch.h"

time_t g_time;
atomic32_t mass_bcp_num;
atomic32_t case_bcp_num;


static int daemon_flag = 1;

int main(int argc, char *argv[])
{
    int ret;
    pthread_t pid_sock;
    pthread_t pid_svm;
    pthread_t pid_mobile;
    pthread_t pid_voip_a;
    pthread_t pid_voip_b;
    pthread_t pid_mass_ftp;
    pthread_t pid_case_ftp;
    pthread_t pid_zip_ftp;
    pthread_t pid_online_dcl;
    pthread_t pid_online_svm;
    pthread_t pid_online_mobile;
    pthread_t pid_online_voip_a;
    pthread_t pid_online_voip_b;
    pthread_t pid_periodic_task;
    pthread_t pid_lispr;

    if(argc == 2 && (strcmp(argv[1], "-v") == 0))
    {
        printf("version: %s\n", VERSION);
        exit(EXIT_SUCCESS);
    }

    open_applog("cfu", LOG_NDELAY, DEF_APP_SYSLOG_FACILITY);
    applog_set_debug_mask(0);

    if (daemon_flag == 1) {
        daemonize();
    }

    init_signal_catch();

    param_parser(argc, argv);
    get_dms_conf();
    ret = get_cfu_conf();
    if (ret)
        return 0;

    register_sguard_server();
    register_ma_server();
    register_rfu_server();

    file_fifo_init();
    mobmass_fifo_init();
    mobonline_array_init();
    init_atomic_counter();
    check_transfer_dir();

    applog(LOG_INFO, APP_LOG_MASK_BASE, "create sock_server thread");
    if (pthread_create(&pid_sock, NULL, (void *)sock_clients_proc, NULL) != 0) {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "thread sock_server create failed");
    }
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_mass_ftp_upload");
    if (pthread_create(&pid_mass_ftp, NULL, (void *)thread_mass_ftp_upload, &mass_file_fifo) != 0) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_mass_ftp_upload create failed");
    }
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_case_ftp_upload");
    if (pthread_create(&pid_case_ftp, NULL, (void *)thread_case_ftp_upload, &case_file_fifo) != 0) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_case_ftp_upload create failed");
    }
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_zip_ftp_upload");
    if (pthread_create(&pid_zip_ftp, NULL, (void *)thread_zip_ftp_upload, NULL) != 0) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_zip_ftp_upload create failed");
    }
    if (svm_enable_flag == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_svm_cdr");
        if (pthread_create(&pid_svm, NULL, (void *)thread_svm_cdr, NULL) != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_svm_cdr create failed");
        }
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_svm_online");
        if (pthread_create(&pid_online_svm, NULL, (void *)thread_svm_online, NULL) != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_svm_online create failed");
        }
    }

    if (mobile_enable_flag == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_mobile_cdr");
        if (pthread_create(&pid_mobile, NULL, (void *)thread_mobile_cdr, NULL) != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_mobile_cdr create failed");
        }
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_mobile_online");
        if (pthread_create(&pid_online_mobile, NULL, (void *)thread_mobile_online, NULL) != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_mobile_online create failed");
        }
    }
    if (voip_a_enable_flag == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_voip_a_cdr");
        if (pthread_create(&pid_voip_a, NULL, (void *)thread_voipa_cdr, NULL) != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_voipa_cdr create failed");
        }
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_voipa_online");
        if (pthread_create(&pid_online_voip_a, NULL, (void *)thread_voipa_online, NULL) != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_voipa_online create failed");
        }
    }
    if (voip_b_enable_flag == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_voipb_cdr");
        if (pthread_create(&pid_voip_b, NULL, (void *)thread_voipb_cdr, NULL) != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_voipb_cdr create failed");
        }
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_voipb_online");
        if (pthread_create(&pid_online_voip_b, NULL, (void *)thread_voipb_online, NULL) != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_voipb_online create failed");
        }
    }
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_dcl_online");
    if (pthread_create(&pid_online_dcl, NULL, (void *)thread_dcl_online, NULL) != 0) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_dcl_online create failed");
    }
    if (lis_enable_flag == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_lispr");
        if (pthread_create(&pid_lispr, NULL, (void *)thread_lispr, NULL) != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_lispr create failed");
        }
    }
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "create thread_periodic_task");
    if (pthread_create(&pid_periodic_task, NULL, (void *)thread_periodic_task, NULL) != 0) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "thread thread_periodic_task create failed");
    }

    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }
        g_time = time(NULL);
        sleep(1);
    }

    pthread_join(pid_sock, NULL);
    pthread_join(pid_mass_ftp, NULL);
    pthread_join(pid_case_ftp, NULL);
    if (svm_enable_flag == 1) {
        pthread_join(pid_svm, NULL);
        pthread_join(pid_online_svm, NULL);
    }
    if (mobile_enable_flag == 1) {
        pthread_join(pid_mobile, NULL);
        pthread_join(pid_online_mobile, NULL);
    }
    pthread_join(pid_online_dcl, NULL);
    if (lis_enable_flag == 1) {
        pthread_join(pid_lispr, NULL);
    }
    pthread_join(pid_periodic_task, NULL);

    return 0;
}

