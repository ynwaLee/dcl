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
#include "zip_ftp_upload.h"
#include "conf_parser.h"
#include "counter.h"
#include "svm_cdr.h"
#include "../../common/applog.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"


void *thread_zip_ftp_upload(void *arg)
{
    char filebackup_local[FILENAME_LEN];
    char filetransfer_local[FILENAME_LEN];
    char fileupload[FILENAME_LEN];
    char fileupload_rename[FILENAME_LEN];
    netbuf *zip_ftp_net;
    int return_value;
    time_t last_time;
    struct tm *tms;
    struct tm tms_buf;
    char zip_dir[1024];
    DIR *fp_dir = NULL;
    struct dirent dir;
    struct dirent *dirp = NULL;
    //struct timeval tv1,tv2;
    //struct timezone tz1,tz2;

    zip_ftp_net = NULL;
    last_time = g_time;

    snprintf(zip_dir, 1023, "%s/%s", cfu_conf.local_transfer_dir, ZIP_DIR);
    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }
        if (zip_ftp_net == NULL) {
            if (FtpConnect(cfu_conf.remote_zip_ftp_ip, &zip_ftp_net) == 1) {
                if (FtpLogin(cfu_conf.remote_zip_ftp_user, cfu_conf.remote_zip_ftp_passwd, zip_ftp_net) == 0) {
                    FtpQuit(zip_ftp_net);
                    zip_ftp_net = NULL;
                    applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, "%s/%s login failure  %s", 
                            cfu_conf.remote_zip_ftp_user, cfu_conf.remote_zip_ftp_passwd, FtpLastResponse(zip_ftp_net));
                    sleep(5);
                    continue;
                } else {
            if (FtpChdir(cfu_conf.remote_zip_dir, zip_ftp_net) != 1) {
                        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, "ftp change zip dir %s failed, exit", cfu_conf.remote_zip_dir);
                        FtpQuit(zip_ftp_net);
                        zip_ftp_net = NULL;
                        exit(-1);
                    }
                }
            } else {
                zip_ftp_net = NULL;
                applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, 
                        "Unable to connect to ftp %s", cfu_conf.remote_zip_ftp_ip);
                sleep(5);
                continue;
            }
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_FTP, "connect to ftp %s success", cfu_conf.remote_zip_ftp_ip);
        }

        if (g_time - last_time >= 10 && zip_ftp_net != NULL) {
            if (FtpChdir(cfu_conf.remote_zip_dir, zip_ftp_net) == 1) {
                applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_FTP, "zip ftp %s connect is alive", cfu_conf.remote_zip_ftp_ip);
            } else {
                applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_FTP, 
                        "change zip ftp %s dir to %s failed, connect is not alive", 
                        cfu_conf.remote_zip_ftp_ip, cfu_conf.remote_zip_dir);
                FtpQuit(zip_ftp_net);
                zip_ftp_net = NULL;
            }
            last_time = g_time;
        }

        if (fp_dir == NULL) {
            fp_dir = opendir(zip_dir);
            if (fp_dir == NULL) {
                applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "open %s failed, %s", zip_dir, strerror(errno));
                continue;
            }
        }
        if (readdir_r(fp_dir, &dir, &dirp) != 0) {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "read %s error, %s", zip_dir, strerror(errno));
            closedir(fp_dir);
            fp_dir = NULL;
            continue;
        }
        if (dirp == NULL) {
            rewinddir(fp_dir);
            usleep(1);
            continue;
        }
        if (dir.d_name[0] == '.' || dir.d_name[0] == 't' || check_filetype(dir.d_name, ".zip") != 0) {
            continue;
        }

        //gettimeofday(&tv1,&tz1);
        snprintf(filetransfer_local, FILENAME_LEN, "%s/%s", zip_dir, dir.d_name);
        snprintf(fileupload, FILENAME_LEN, "%s/%s.tmp", cfu_conf.remote_zip_dir, dir.d_name);
        return_value = FtpPut(filetransfer_local, fileupload, FTPLIB_BINARY, zip_ftp_net);
        if (return_value == 1) {
            snprintf(fileupload_rename, FILENAME_LEN, "%s/%s", cfu_conf.remote_zip_dir, dir.d_name);
            return_value = FtpRename(fileupload, fileupload_rename, zip_ftp_net);
            if (return_value == 1) {
                atomic64_add(&das_upload_success, 1);
                //backup
                tms = localtime_r(&g_time, &tms_buf);
                if (tms != NULL) {
                    snprintf(filebackup_local, FILENAME_LEN, "%s/%04d-%02d-%02d/%02d/%s", 
                            cfu_conf.local_backup_dir, tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, dir.d_name);
                    rename(filetransfer_local, filebackup_local);
                }

            } else {
                applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, "ftp rename %s failed", fileupload);
                FtpQuit(zip_ftp_net);
                zip_ftp_net = NULL;
            }
        } else {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_FTP, "ftp upload %s failed", filetransfer_local);
            FtpQuit(zip_ftp_net);
            zip_ftp_net = NULL;
        }
        //gettimeofday(&tv2,&tz2);
        //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "ftp upload case bcp spent %lu usec.", 
                //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
    }

    if (zip_ftp_net != NULL) {
        FtpQuit(zip_ftp_net);
    }

    pthread_exit(NULL);
}








