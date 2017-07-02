#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/inotify.h>
#include "atomic.h"
#include "cfu.h"
#include "conf_parser.h"
#include "svm_cdr.h"
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

//#define SVM_DIR     "svm"
#define SVM_DIR     "phone"


extern atomic32_t mass_bcp_num;
extern atomic32_t case_bcp_num;


int check_svm_dir(void)
{
    struct stat dir_stat;
    char cmd[1024];
    char path[1024];

    snprintf(path, 1023, "%s/%s", cfu_conf.local_ftp_dir, SVM_DIR);

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


int check_filetype(char *filename, char *type)
{
    int len_name;
    int len_type;
    int return_value;

    if(filename == NULL || type == NULL) {
        return -1;
    }

    len_name = strlen(filename);
    len_type = strlen(type);
    if (len_type >= len_name) {
        return -1;
    }
    return_value = memcmp(filename + (len_name - len_type), type, len_type);

    return return_value;
}




int svm_voice_parser(char *buffer, voice_record_t *svm_record)
{
    int n;
    
    n = sscanf(buffer, 
            "%lu\t%u\t%d\t%d\t%d\t%lu\t%lu\t%lu\t%d\t%s\t%s\t%d\t%s\t%s\t%d\t%d\t%d\t%d\t%u\t%u\t%u\t%u\t%u\t%d\t%d\n", 
            &svm_record->callid, 
            &svm_record->ruleid, 
            &svm_record->datasrcid, 
            &svm_record->status, 
            &svm_record->path, 
            &svm_record->dialtime, 
            &svm_record->starttime, 
            &svm_record->endtime, 
            &svm_record->duration, 
            svm_record->fromphone, 
            svm_record->tophone, 
            &svm_record->linklayerid, 
            svm_record->inunit, 
            svm_record->outunit, 
            &svm_record->recordflag, 
            &svm_record->callflag, 
            &svm_record->remarkflag, 
            &svm_record->itemflag, 
            &svm_record->userid, 
            &svm_record->voiceid, 
            &svm_record->percent, 
            &svm_record->stage, 
            &svm_record->vrsflag, 
            &svm_record->i1,
            &svm_record->i2);
       
        return n;
}


int svm_sms_parser(char *buffer, sms_record_t *svm_record)
{
    int n;
    
    n = sscanf(buffer, 
            "%lu\t%d\t%d\t%d\t%s\t%lu\t%lu\t%s\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\n", 
            &svm_record->smsid, 
            &svm_record->ruleid, 
            &svm_record->datasrcid, 
            &svm_record->status, 
            svm_record->action, 
            &svm_record->sendtime, 
            &svm_record->recvtime, 
            svm_record->fromphone, 
            svm_record->fromimsi, 
            svm_record->tophone, 
            svm_record->toimsi, 
            svm_record->content, 
            &svm_record->len, 
            &svm_record->recordflag, 
            &svm_record->remarkflag, 
            &svm_record->itemflag, 
            &svm_record->i1,
            &svm_record->i2);

    return n;
}






int svm_massvoice_convert(char *path, char *filename)
{
    FILE *fp_src;
    FILE *fp_dst;
    char buffer[2048] = {0};
    int return_value;
    int bcp_num;
    voice_record_t svm_record;
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
    while (fgets(buffer, 2048, fp_src) != NULL) {
        return_value = svm_voice_parser(buffer, &svm_record);
        if (return_value == 25) {
            atomic64_add(&svm_mass_call_num, 1);
        
            svm2das_mass_voice(&svm_record, &das_record);

            write_bcp(fp_dst, &das_record);

            atomic64_add(&das_mass_call_num, 1);
            cdr_counter += 1;
        }
    }
    //applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "svm mass voice convert %d cdr", cdr_counter);

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


int svm_masssms_convert(char *path, char *filename)
{
    FILE *fp_src;
    FILE *fp_dst;
    char buffer[4096] = {0};
    int return_value;
    int bcp_num;
    sms_record_t svm_record;
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
    while (fgets(buffer, 4096, fp_src) != NULL) {
        return_value = svm_sms_parser(buffer, &svm_record);
        if (return_value == 18) {
            atomic64_add(&svm_mass_call_num, 1);
        
            svm2das_mass_sms(&svm_record, &das_record);

            write_bcp(fp_dst, &das_record);
            atomic64_add(&das_mass_call_num, 1);
            cdr_counter += 1;
        }
    }
    //applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "svm mass sms convert %d cdr", cdr_counter);

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


int svm_casevoice_convert(char *path, char *filename)
{
    FILE *fp_src;
    FILE *fp_dst;
    char buffer[2048] = {0};
    int return_value;
    int bcp_num;
    int object_id;
    int clue_id;
    uint32_t random_num;
    voice_record_t svm_record;
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
    while(fgets(buffer, 2048, fp_src) != NULL) {
        return_value = svm_voice_parser(buffer, &svm_record);
        if (return_value != 25) {
            continue;
        }

        atomic64_add(&svm_case_call_num, 1);
        clue_id = svm_record.ruleid;
        object_id = get_object(clue_id);
        if (object_id == 0) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "clue_id %d no matching object_id", clue_id);
            continue;
        }
        
        memset(&file_node, 0, sizeof(file_node_t));
        bcp_num = atomic32_add_return(&case_bcp_num, 1);
        random_num = (g_time << 16) | (bcp_num & 0xffff);
        snprintf(file_node.filename, 256, "CASE_%u_CALL_%u.bcp", random_num, object_id);
        snprintf(filetransfer_tmp, 512, "%s/%s/%s.tmp", cfu_conf.local_transfer_dir, CASE_DIR, file_node.filename);
        snprintf(file_node.file_transfer, 512, "%s/%s/%s", cfu_conf.local_transfer_dir, CASE_DIR, file_node.filename);
        file_node.case_flag = 1;
        
        fp_dst = fopen(filetransfer_tmp, "w+");
        if (fp_dst == NULL) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", filetransfer_tmp, strerror(errno));
            continue;
        }

        svm2das_case_voice(&svm_record, &das_record, object_id);
        
        write_bcp(fp_dst, &das_record);
        atomic64_add(&das_case_call_num, 1);
        cdr_counter += 1;

        fclose(fp_dst);
        rename(filetransfer_tmp, file_node.file_transfer);
        atomic64_add(&das_case_file_num, 1);
    }
    //applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "svm case voice convert %d cdr", cdr_counter);

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

int svm_casesms_convert(char *path, char *filename)
{
    FILE *fp_src;
    FILE *fp_dst;
    char buffer[4096] = {0};
    int return_value;
    int bcp_num;
    int object_id;
    int clue_id;
    uint32_t random_num;
    sms_record_t svm_record;
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
    while(fgets(buffer, 4096, fp_src) != NULL) {
        return_value = svm_sms_parser(buffer, &svm_record);
        if (return_value != 18) {
            continue;
        }

        atomic64_add(&svm_case_call_num, 1);
        clue_id = svm_record.ruleid;
        object_id = get_object(clue_id);
        if (object_id == 0) {
            continue;
        }
        
        memset(&file_node, 0, sizeof(file_node_t));
        bcp_num = atomic32_add_return(&case_bcp_num, 1);
        random_num = (g_time << 16) | (bcp_num & 0xffff);
        snprintf(file_node.filename, 256, "CASE_%u_CALL_%u.bcp", random_num, object_id);
        snprintf(filetransfer_tmp, 512, "%s/%s/%s.tmp", cfu_conf.local_transfer_dir, CASE_DIR, file_node.filename);
        snprintf(file_node.file_transfer, 512, "%s/%s/%s", cfu_conf.local_transfer_dir, CASE_DIR, file_node.filename);
        file_node.case_flag = 1;
        
        fp_dst = fopen(filetransfer_tmp, "w+");
        if (fp_dst == NULL) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", filetransfer_tmp, strerror(errno));
            continue;
        }

        svm2das_case_sms(&svm_record, &das_record, object_id);
        
        write_bcp(fp_dst, &das_record);
        atomic64_add(&das_case_call_num, 1);
        cdr_counter += 1;

        fclose(fp_dst);
        rename(filetransfer_tmp, file_node.file_transfer);
        atomic64_add(&das_case_file_num, 1);
    }
    //applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "svm case sms convert %d cdr", cdr_counter);

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




void *thread_svm_cdr(void *arg)
{
    char svm_dir[1024];
    DIR *fp_dir = NULL;
    struct dirent dir;
    struct dirent *dirp = NULL;
    //struct timeval tv1,tv2;
    //struct timezone tz1,tz2;

    check_svm_dir();

    snprintf(svm_dir, 1023, "%s/%s", cfu_conf.local_ftp_dir, SVM_DIR);

    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }

        if (fp_dir == NULL) {
            fp_dir = opendir(svm_dir);
            if (fp_dir == NULL) {
                applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "open %s failed, %s", svm_dir, strerror(errno));
                continue;
            }
        }

        if (readdir_r(fp_dir, &dir, &dirp) != 0) {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "read %s error, %s", svm_dir, strerror(errno));
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

        if (check_filetype(dir.d_name, ".txt") == 0) {
            if (strncmp(dir.d_name, "mass-voice", 10) == 0) {
                //gettimeofday(&tv1,&tz1);
                svm_massvoice_convert(svm_dir, dir.d_name);
                atomic64_add(&svm_mass_file_num, 1);
                //gettimeofday(&tv2,&tz2);
                //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "svm mass voice cdr convert to bcp spent %lu usec.", 
                        //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
            } else if (strncmp(dir.d_name, "mass-sms", 8) == 0) {
                //gettimeofday(&tv1,&tz1);
                svm_masssms_convert(svm_dir, dir.d_name);
                atomic64_add(&svm_mass_file_num, 1);
                //gettimeofday(&tv2,&tz2);
                //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "svm mass sms cdr convert to bcp spent %lu usec.", 
                        //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
            } else if (strncmp(dir.d_name, "rule-voice", 10) == 0) {
                //gettimeofday(&tv1,&tz1);
                svm_casevoice_convert(svm_dir, dir.d_name);
                atomic64_add(&svm_case_file_num, 1);
                //gettimeofday(&tv2,&tz2);
                //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "svm case voice cdr convert to bcp spent %lu usec.", 
                        //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
            } else if (strncmp(dir.d_name, "rule-sms", 8) == 0) {
                //gettimeofday(&tv1,&tz1);
                svm_casesms_convert(svm_dir, dir.d_name);
                atomic64_add(&svm_case_file_num, 1);
                //gettimeofday(&tv2,&tz2);
                //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "svm case sms cdr convert to bcp spent %lu usec.", 
                        //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
            }
        }

    }

    closedir(fp_dir);
    pthread_exit(NULL);
}


