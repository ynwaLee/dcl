#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/inotify.h>
#include "atomic.h"
#include "cfu.h"
#include "conf_parser.h"
#include "svm_cdr.h"
#include "voipb_cdr.h"
#include "convert2das.h"
#include "file_fifo.h"
#include "ftplib.h"
#include "rfu_client.h"
#include "counter.h"
#include "mass_ftp_upload.h"
#include "case_ftp_upload.h"
#include "../../common/applog.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"


#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 64 ) )
#define VOIPB_DIR   "voip-b"





extern atomic32_t mass_bcp_num;
extern atomic32_t case_bcp_num;






int check_voipb_dir(void)
{
    struct stat dir_stat;
    char cmd[1024];
    char path[1024];

    snprintf(path, 1023, "%s/%s", cfu_conf.local_ftp_dir, VOIPB_DIR);

    if (stat(path, &dir_stat) != 0 || (dir_stat.st_mode & S_IFMT) != S_IFDIR) {
        snprintf(cmd, 1024, "mkdir -p %s", path);
        system(cmd);
    }

    snprintf(cmd, 1024, "chown -R %s %s", cfu_conf.local_ftp_user, path);
    system(cmd);
    snprintf(cmd, 1024, "chmod -R 777 %s", path);
    system(cmd);

    return 0;
}






int voipb_mass_parser(char *buf, voipb_mass_t *mass)
{
    char *p;
    char *q;
    int n;

    p = buf;
    n = 0;
    memset(mass, 0, sizeof(voipb_mass_t));
    while (1) {
        q = strchr(p, ',');
        if (q == NULL) {
            break;
        }
        n++;
        switch (n) {
            case 1:
                strncpy(mass->aisdn, p + 1, q - p - 2);
                break;
            case 2:
                strncpy(mass->ahome, p + 1, q - p - 2);
                break;
            case 3:
                strncpy(mass->aimsi, p + 1, q - p - 2);
                break;
            case 4:
                strncpy(mass->aimei, p + 1, q - p - 2);
                break;
            case 5:
                strncpy(mass->aneid, p + 1, q - p - 2);
                break;
            case 6:
                strncpy(mass->amcc, p + 1, q - p - 2);
                break;
            case 7:
                strncpy(mass->amnc, p + 1, q - p - 2);
                break;
            case 8:
                strncpy(mass->alacs, p + 1, q - p - 2);
                break;
            case 9:
                strncpy(mass->acells, p + 1, q - p - 2);
                break;
            case 10:
                strncpy(mass->alace, p + 1, q - p - 2);
                break;
            case 11:
                strncpy(mass->acelle, p + 1, q - p - 2);
                break;
            case 12:
                strncpy(mass->apc, p + 1, q - p - 2);
                break;
            case 13:
                strncpy(mass->bisdn, p + 1, q - p - 2);
                break;
            case 14:
                strncpy(mass->bisdnx, p + 1, q - p - 2);
                break;
            case 15:
                strncpy(mass->bhome, p + 1, q - p - 2);
                break;
            case 16:
                strncpy(mass->btisdn, p + 1, q - p - 2);
                break;
            case 17:
                strncpy(mass->bimsi, p + 1, q - p - 2);
                break;
            case 18:
                strncpy(mass->bneid, p + 1, q - p - 2);
                break;
            case 19:
                strncpy(mass->bpc, p + 1, q - p - 2);
                break;
            case 20:
                strncpy(mass->acttype, p + 1, q - p - 2);
                break;
            case 21:
                strncpy(mass->starttime, p + 1, q - p - 2);
                break;
            case 22:
                strncpy(mass->endtime, p + 1, q - p - 2);
                break;
            case 23:
                strncpy(mass->duration, p + 1, q - p - 2);
                break;
            case 24:
                strncpy(mass->vocfile, p + 1, q - p - 2);
                break;
            case 25:
                strncpy(mass->extinfo, p + 1, q - p - 2);
                break;
            case 26:
                strncpy(mass->result, p + 1, q - p - 2);
                break;
            case 27:
                strncpy(mass->cause, p + 1, q - p - 2);
                break;
            case 28:
                strncpy(mass->netid, p + 1, q - p - 2);
                break;
            case 29:
                strncpy(mass->smslan, p + 1, q - p - 2);
                break;
            case 30:
                strncpy(mass->smstext, p + 1, q - p - 2);
                break;
        }
        p = q + 1;
    }
    if (n == 29) {
        strncpy(mass->smstext, p + 1, strlen(p) - 4);
        n++;
    }

    return n;
}



int voipb_case_parser(char *buf, voipb_case_t *cases)
{
    char *p;
    char *q;
    int n;

    p = buf;
    n = 0;
    memset(cases, 0, sizeof(voipb_case_t));
    while (1) {
        q = strchr(p, ',');
        if (q == NULL) {
            break;
        }
        n++;
        switch (n) {
            case 1:
                strncpy(cases->mntid, p + 1, q - p - 2);
                break;
            case 2:
                strncpy(cases->aisdn, p + 1, q - p - 2);
                break;
            case 3:
                strncpy(cases->ahome, p + 1, q - p - 2);
                break;
            case 4:
                strncpy(cases->aimsi, p + 1, q - p - 2);
                break;
            case 5:
                strncpy(cases->aimei, p + 1, q - p - 2);
                break;
            case 6:
                strncpy(cases->aneid, p + 1, q - p - 2);
                break;
            case 7:
                strncpy(cases->amcc, p + 1, q - p - 2);
                break;
            case 8:
                strncpy(cases->amnc, p + 1, q - p - 2);
                break;
            case 9:
                strncpy(cases->alacs, p + 1, q - p - 2);
                break;
            case 10:
                strncpy(cases->acells, p + 1, q - p - 2);
                break;
            case 11:
                strncpy(cases->alace, p + 1, q - p - 2);
                break;
            case 12:
                strncpy(cases->acelle, p + 1, q - p - 2);
                break;
            case 13:
                strncpy(cases->apc, p + 1, q - p - 2);
                break;
            case 14:
                strncpy(cases->bisdn, p + 1, q - p - 2);
                break;
            case 15:
                strncpy(cases->bisdnx, p + 1, q - p - 2);
                break;
            case 16:
                strncpy(cases->bhome, p + 1, q - p - 2);
                break;
            case 17:
                strncpy(cases->btisdn, p + 1, q - p - 2);
                break;
            case 18:
                strncpy(cases->bimsi, p + 1, q - p - 2);
                break;
            case 19:
                strncpy(cases->bneid, p + 1, q - p - 2);
                break;
            case 20:
                strncpy(cases->bpc, p + 1, q - p - 2);
                break;
            case 21:
                strncpy(cases->acttype, p + 1, q - p - 2);
                break;
            case 22:
                strncpy(cases->starttime, p + 1, q - p - 2);
                break;
            case 23:
                strncpy(cases->endtime, p + 1, q - p - 2);
                break;
            case 24:
                strncpy(cases->duration, p + 1, q - p - 2);
                break;
            case 25:
                strncpy(cases->vocfile, p + 1, q - p - 2);
                break;
            case 26:
                strncpy(cases->extinfo, p + 1, q - p - 2);
                break;
            case 27:
                strncpy(cases->result, p + 1, q - p - 2);
                break;
            case 28:
                strncpy(cases->cause, p + 1, q - p - 2);
                break;
            case 29:
                strncpy(cases->netid, p + 1, q - p - 2);
                break;
            case 30:
                strncpy(cases->smslan, p + 1, q - p - 2);
                break;
        }
        p = q + 1;
    }
    if (n == 30) {
        strncpy(cases->smstext, p + 1, strlen(p) - 4);
        n++;
    }

    return n;
}








int voipb_mass_convert(char *path, char *filename)
{
    FILE *fp_src;
    FILE *fp_dst;
    char buffer[1024] = {0};
    int return_value;
    int bcp_num;
    voipb_mass_t voipb_mass;
    das_record_t das_record;
    file_node_t file_node;
    char file_local[FILENAME_LEN];
    char filebackup_local[FILENAME_LEN];
    char filetransfer_tmp[FILENAME_LEN];
    struct tm *tms;
    struct tm tms_buf;
    int cdr_counter;

    if (path == NULL || filename == NULL) {
        return -1;
    }

    snprintf(file_local, FILENAME_LEN, "%s/%s", path, filename);

    memset(&file_node, 0, sizeof(file_node_t));
    bcp_num = atomic32_add_return(&mass_bcp_num, 1);
    snprintf(file_node.filename, 256, "MASS_%d_%d_CALL_1_111_1.bcp", (int)g_time, bcp_num);
    snprintf(filetransfer_tmp, 512, "%s/%s/%s.tmp", cfu_conf.local_transfer_dir, MASS_DIR, file_node.filename);
    snprintf(file_node.file_transfer, 512, "%s/%s/%s", cfu_conf.local_transfer_dir, MASS_DIR, file_node.filename);
    file_node.case_flag = 0;

    fp_src = fopen(file_local, "r");
    if (fp_src == NULL) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", file_local, strerror(errno));
        return -1;
    }
    fp_dst = fopen(filetransfer_tmp, "w+");
    if (fp_dst == NULL) {
        fclose(fp_src);
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", filetransfer_tmp, strerror(errno));
        return -1;
    }

    cdr_counter = 0;
    while(fgets(buffer, 1024, fp_src) != NULL) {
        return_value = voipb_mass_parser(buffer, &voipb_mass);
        if (return_value == 30) {
            atomic64_add(&voipb_mass_call_num, 1);
            voipb2das_mass(&voipb_mass, &das_record);
            write_bcp(fp_dst, &das_record);
            atomic64_add(&das_mass_call_num, 1);
            cdr_counter += 1;
        }
    }
    //applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "voipb mass convert %d cdr", cdr_counter);

    fclose(fp_src);
    fclose(fp_dst);
    rename(filetransfer_tmp, file_node.file_transfer);

    atomic64_add(&das_mass_file_num, 1);
    //backup file
    tms = localtime_r(&g_time, &tms_buf);
    if (tms == NULL) {
        return -1;
    }
    snprintf(filebackup_local, FILENAME_LEN, "%s/%04d-%02d-%02d/%02d/%s", 
            cfu_conf.local_backup_dir, tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, filename);
    rename(file_local, filebackup_local);

    return 0;
}


int voipb_case_convert(char *path, char *filename)
{
    FILE *fp_src;
    FILE *fp_dst;
    char buffer[1024] = {0};
    int return_value;
    int bcp_num;
    int object_id;
    int clue_id;
    uint32_t random_num;
    voipb_case_t voipb_case;
    das_record_t das_record;
    file_node_t file_node;
    char file_local[FILENAME_LEN];
    char filebackup_local[FILENAME_LEN];
    char filetransfer_tmp[FILENAME_LEN];
    struct tm *tms;
    struct tm tms_buf;
    int cdr_counter;

    if (path == NULL || filename == NULL) {
        return -1;
    }
    snprintf(file_local, FILENAME_LEN, "%s/%s", path, filename);

    fp_src = fopen(file_local, "r");
    if (fp_src == NULL) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", file_local, strerror(errno));
        return -1;
    }

    cdr_counter = 0;
    while(fgets(buffer, 1024, fp_src) != NULL) {
        return_value = voipb_case_parser(buffer, &voipb_case);
        if (return_value != 31) {
            continue;
        }

        atomic64_add(&voipb_case_call_num, 1);
        clue_id = atoi(voipb_case.mntid) - 200;
        object_id = get_object(clue_id);
        if (object_id == 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "clue id %d does not match any object id", clue_id);
            continue;
        }
        
        memset(&file_node, 0, sizeof(file_node_t));
        bcp_num = atomic32_add_return(&case_bcp_num, 1);
        random_num = (g_time << 16) | (bcp_num & 0xffff);
        snprintf(file_node.filename, 256, "CASE_%u_CALL_%u.bcp", random_num, object_id);
        snprintf(filetransfer_tmp, 512, "%s/%s/%s", cfu_conf.local_transfer_dir, CASE_DIR, file_node.filename);
        snprintf(file_node.file_transfer, 512, "%s/%s/%s", cfu_conf.local_transfer_dir, CASE_DIR, file_node.filename);
        file_node.case_flag = 1;
        
        fp_dst = fopen(filetransfer_tmp, "w+");
        if (fp_dst == NULL) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", filetransfer_tmp, strerror(errno));
            continue;
        }

        voipb2das_case(&voipb_case, &das_record, object_id);

        write_bcp(fp_dst, &das_record);
        atomic64_add(&das_case_call_num, 1);
        cdr_counter += 1;

        fclose(fp_dst);
        rename(filetransfer_tmp, file_node.file_transfer);
        atomic64_add(&das_case_file_num, 1);
    }
    //applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "voipb case convert %d cdr", cdr_counter);

    fclose(fp_src);
    //backup file
    tms = localtime_r(&g_time, &tms_buf);
    if (tms == NULL) {
        return -1;
    }
    snprintf(filebackup_local, FILENAME_LEN, "%s/%04d-%02d-%02d/%02d/%s", 
            cfu_conf.local_backup_dir, tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, filename);
    rename(file_local, filebackup_local);

    return 0;
}




int settle_voipb_history_file(void)
{
    char path[1024];
    DIR *fp;
    struct dirent dir;
    struct dirent *dirp = NULL;
    char *offset;

    snprintf(path, 1023, "%s/%s", cfu_conf.local_ftp_dir, VOIPB_DIR);

    fp = opendir(path);
    if (fp == NULL) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "open %s failed", path);
        return -1;
    }

    while (readdir_r(fp, &dir, &dirp) == 0 && dirp != NULL) {
        if (dir.d_name[0] == '.') {
            continue;
        }
        if (check_filetype(dir.d_name, ".csv") == 0) {
            offset = strchr(dir.d_name, '.');
            if (offset != NULL) {
                if (offset - dir.d_name == 12) {
                    //mass
                    voipb_mass_convert(path, dir.d_name);
                    atomic64_add(&voipb_mass_file_num, 1);
                } else if (offset - dir.d_name == 14) {
                    //case
                    voipb_case_convert(path, dir.d_name);
                    atomic64_add(&voipb_case_file_num, 1);
                }
            }
        }
    }

    closedir(fp);

    return 0;
}




void *thread_voipb_cdr(void *arg)
{
    char voipb_dir[1024];
    char *offset;
    DIR *fp_dir = NULL;
    struct dirent dir;
    struct dirent *dirp = NULL;
    //struct timeval tv1,tv2;
    //struct timezone tz1,tz2;


    check_voipb_dir();

    snprintf(voipb_dir, 1023, "%s/%s", cfu_conf.local_ftp_dir, VOIPB_DIR);

    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }

        if (fp_dir == NULL) {
            fp_dir = opendir(voipb_dir);
            if (fp_dir == NULL) {
                applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "open %s failed, %s", voipb_dir, strerror(errno));
                continue;
            } else {
                applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "open %s success", voipb_dir);
            }
        }

        if (readdir_r(fp_dir, &dir, &dirp) != 0) {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "read %s error, %s", voipb_dir, strerror(errno));
            closedir(fp_dir);
            fp_dir = NULL;
            continue;
        }
        if (dirp == NULL) {
            rewinddir(fp_dir);
            usleep(1);
            continue;
        }
        if (dir.d_name[0] == '.') {
            continue;
        }
        
        if (check_filetype(dir.d_name, ".csv") == 0) {
            offset = strchr(dir.d_name, '.');
            if (offset != NULL) {
                if (offset - dir.d_name == 12) {
                    //gettimeofday(&tv1,&tz1);
                    //mass
                    voipb_mass_convert(voipb_dir, dir.d_name);
                    atomic64_add(&voipb_mass_file_num, 1);
                    //gettimeofday(&tv2,&tz2);
                    //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "voipb mass cdr convert to bcp spent %lu usec.", 
                            //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
                } else if (offset - dir.d_name == 14) {
                    //gettimeofday(&tv1,&tz1);
                    //case
                    voipb_case_convert(voipb_dir, dir.d_name);
                    atomic64_add(&voipb_case_file_num, 1);
                    //gettimeofday(&tv2,&tz2);
                    //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "voipb case cdr convert to bcp spent %lu usec.", 
                            //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
                } else {
                    //error
                    applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_TELE, 
                            "voipb file error: %s", dir.d_name);
                }
            }
        }

    }

    closedir(fp_dir);

    pthread_exit(NULL);
}
