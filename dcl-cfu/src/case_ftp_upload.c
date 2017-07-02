#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "cfu.h"
#include "ftplib.h"
#include "file_fifo.h"
#include "case_ftp_upload.h"
#include "conf_parser.h"
#include "counter.h"
#include "svm_cdr.h"
#include "../../common/applog.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


void *thread_case_ftp_upload(void *arg)
{
    char filebackup_local[FILENAME_LEN];
    char filetransfer_local[FILENAME_LEN];
    char fileupload[FILENAME_LEN];
    char fileupload_rename[FILENAME_LEN];
    netbuf *case_ftp_net;
    int return_value;
    time_t last_time;
    struct tm *tms;
    struct tm tms_buf;
    char case_dir[1024];
    DIR *fp_dir = NULL;
    struct dirent dir;
    struct dirent *dirp = NULL;
    //struct timeval tv1,tv2;
    //struct timezone tz1,tz2;

    case_ftp_net = NULL;
    last_time = g_time;

    snprintf(case_dir, 1023, "%s/%s", cfu_conf.local_transfer_dir, CASE_DIR);
    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }
        if (case_ftp_net == NULL) {
            if (FtpConnect(cfu_conf.remote_case_ftp_ip, &case_ftp_net) == 1) {
                if (FtpLogin(cfu_conf.remote_case_ftp_user, cfu_conf.remote_case_ftp_passwd, case_ftp_net) == 0) {
                    FtpQuit(case_ftp_net);
                    case_ftp_net = NULL;
                    applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, "%s/%s login failure  %s", 
                            cfu_conf.remote_case_ftp_user, cfu_conf.remote_case_ftp_passwd, FtpLastResponse(case_ftp_net));
                    sleep(5);
                    continue;
                } else {
            if (FtpChdir(cfu_conf.remote_case_dir, case_ftp_net) != 1) {
                        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, "ftp change case dir %s failed, exit", cfu_conf.remote_case_dir);
                        FtpQuit(case_ftp_net);
                        case_ftp_net = NULL;
                        exit(-1);
                    }
                }
            } else {
                case_ftp_net = NULL;
                applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, 
                        "Unable to connect to ftp %s", cfu_conf.remote_case_ftp_ip);
                sleep(5);
                continue;
            }
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_FTP, "connect to ftp %s success", cfu_conf.remote_case_ftp_ip);
        }

        if (g_time - last_time >= 10 && case_ftp_net != NULL) {
            if (FtpChdir(cfu_conf.remote_case_dir, case_ftp_net) == 1) {
                applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_FTP, "case ftp %s connect is alive", cfu_conf.remote_case_ftp_ip);
            } else {
                applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_FTP, 
                        "change case ftp %s dir to %s failed, connect is not alive", 
                        cfu_conf.remote_case_ftp_ip, cfu_conf.remote_case_dir);
                FtpQuit(case_ftp_net);
                case_ftp_net = NULL;
            }
            last_time = g_time;
        }

        if (fp_dir == NULL) {
            fp_dir = opendir(case_dir);
            if (fp_dir == NULL) {
                applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "open %s failed, %s", case_dir, strerror(errno));
                continue;
            }
        }
        if (readdir_r(fp_dir, &dir, &dirp) != 0) {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "read %s error, %s", case_dir, strerror(errno));
            closedir(fp_dir);
            fp_dir = NULL;
            continue;
        }
        if (dirp == NULL) {
            rewinddir(fp_dir);
            usleep(1);
            continue;
        }
        if (dir.d_name[0] == '.' || check_filetype(dir.d_name, ".bcp") != 0) {
            continue;
        }

        //gettimeofday(&tv1,&tz1);
        snprintf(filetransfer_local, FILENAME_LEN, "%s/%s", case_dir, dir.d_name);
        snprintf(fileupload, FILENAME_LEN, "%s/%s.tmp", cfu_conf.remote_case_dir, dir.d_name);
        return_value = FtpPut(filetransfer_local, fileupload, FTPLIB_BINARY, case_ftp_net);
        if (return_value == 1) {
            applog( APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[FTP PUT SUCESS] %s", fileupload);
            snprintf(fileupload_rename, FILENAME_LEN, "%s/%s", cfu_conf.remote_case_dir, dir.d_name);
            return_value = FtpRename(fileupload, fileupload_rename, case_ftp_net);
            if (return_value == 1) {
                applog( APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[FTP RENAME SUCESS] %s", fileupload_rename);
                atomic64_add(&das_upload_success, 1);
                //backup
                tms = localtime_r(&g_time, &tms_buf);
                if (tms != NULL) {
                    snprintf(filebackup_local, FILENAME_LEN, "%s/%04d-%02d-%02d/%02d/%s", 
                            cfu_conf.local_backup_dir, tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, dir.d_name);
                    rename(filetransfer_local, filebackup_local);
                }

            } else {
                applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, "[FTP RENAME FAILED] %s", fileupload);
                FtpQuit(case_ftp_net);
                case_ftp_net = NULL;
            }
        } else {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, "[FTP PUT FAILED] %s", filetransfer_local);
            FtpQuit(case_ftp_net);
            case_ftp_net = NULL;
        }
        //gettimeofday(&tv2,&tz2);
        //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "ftp upload case bcp spent %lu usec.", 
                //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
    }

    if (case_ftp_net != NULL) {
        FtpQuit(case_ftp_net);
    }

    pthread_exit(NULL);
}








