#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "cfu.h"
#include "ma_cli.h"
#include "conf_parser.h"
#include "../../common/get_arg.h"
#include "../../common/applog.h"
#include "../../common/conf.h"

#define     FILENAME_LEN    1024


STRUCT_CFU_CONF_T cfu_conf;

char g_config_path[DIR_LEN] = DEFAULT_CONFIG_PATH;

extern MA_INTF_CONF ma_intf_conf;


int svm_enable_flag = 0;
int mobile_enable_flag = 0;
int voip_a_enable_flag = 0;
int voip_b_enable_flag = 0;
int lis_enable_flag = 0;

int check_exist_dirs(char *path)
{
    struct stat dir_stat;
    if (path == NULL) {
        return -1;
    }
    if (stat(path, &dir_stat) == 0 && (dir_stat.st_mode & S_IFMT) == S_IFDIR) {
        return 0;
    }  else {
        return -1;
    }

    return 0;
}

int check_transfer_dir(void)
{
    struct stat dir_stat;
    char cmd[1024];
    char path[1024];

    snprintf(path, 1023, "%s/%s", cfu_conf.local_transfer_dir, MASS_DIR);
    if (stat(path, &dir_stat) != 0 || (dir_stat.st_mode & S_IFMT) != S_IFDIR) {
        snprintf(cmd, 1024, "mkdir -p %s", path);
        system(cmd);
    }

    snprintf(path, 1023, "%s/%s", cfu_conf.local_transfer_dir, CASE_DIR);
    if (stat(path, &dir_stat) != 0 || (dir_stat.st_mode & S_IFMT) != S_IFDIR) {
        snprintf(cmd, 1024, "mkdir -p %s", path);
        system(cmd);
    }

    snprintf(path, 1023, "%s/%s", cfu_conf.local_transfer_dir, ZIP_DIR);
    if (stat(path, &dir_stat) != 0 || (dir_stat.st_mode & S_IFMT) != S_IFDIR) {
        snprintf(cmd, 1024, "mkdir -p %s", path);
        system(cmd);
    }

    return 0;
}



int create_backup_dirs(char *root, int year, int month, int day)
{
    struct stat dir_stat;
    char date_dir[1024] = {0};
    char hour_dir[1024] = {0};
    int ret;
    int i;

    if (root == NULL) {
        return -1;
    }

    snprintf(date_dir, 1024, "%s/%04d-%02d-%02d", root, year, month, day);
    if (stat(date_dir, &dir_stat) != 0 || (dir_stat.st_mode & S_IFMT) != S_IFDIR) {
        ret = mkdir(date_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (ret != 0) {
            applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "mkdir %s error", date_dir);
            return -1;
        }
    }

    for (i = 0; i < 24; i++) {
        snprintf(hour_dir, 1024, "%s/%02d", date_dir, i);
        if (stat(hour_dir, &dir_stat) != 0 || (dir_stat.st_mode & S_IFMT) != S_IFDIR) {
            ret = mkdir(hour_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (ret != 0) {
                applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "mkdir %s error", hour_dir);
            }
        }
    }

    return 0;
}


int cleanup_backup_dirs(char *root)
{
    DIR *dirp;
    struct dirent *dir;
    char backup_dirs[8][FILENAME_LEN];
    int backup_dirs_num = 0;
    time_t last_time;
    struct tm *tms;
    struct tm tms_buf;
    char cmd[1024];
    int i;

    if (root == NULL) {
        return -1;
    }

    dirp = opendir(root);
    if (dirp == NULL) {
        applog(APP_LOG_LEVEL_ERR, APP_VPU_LOG_MASK_BASE, "opendir %s error", root);
        return -1;
    }

    backup_dirs_num = 0;
    for (i = 0; i < cfu_conf.local_backup_time + 1; i++) {
        last_time = g_time - (60 * 60 * 24) * i;
        tms = localtime_r(&last_time, &tms_buf);
        if (tms == NULL) {
            continue;
        }
        snprintf(backup_dirs[i], FILENAME_LEN, "%04d-%02d-%02d", tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday);
        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "keep %s", backup_dirs[i]);
        backup_dirs_num += 1;
    }

    last_time = g_time + 60 * 60 * 24;
    tms = localtime_r(&last_time, &tms_buf);
    if (tms != NULL) {
        snprintf(backup_dirs[i], FILENAME_LEN, "%04d-%02d-%02d", tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday);
        backup_dirs_num += 1;
        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "keep %s", backup_dirs[i]);
    }

    while (NULL != (dir = readdir(dirp))) {
        if (dir->d_type == DT_DIR && dir->d_name[0] != '.') {
            for (i = 0; i < backup_dirs_num; i++) {
                if (strcmp(dir->d_name, backup_dirs[i]) == 0) {
                    break;
                }
            }
            if (i >= backup_dirs_num) {
                snprintf(cmd, 1024, "rm -rf %s/%s", root, dir->d_name);
                system(cmd);
                applog(APP_LOG_LEVEL_NOTICE, APP_VPU_LOG_MASK_BASE, "cleanup %s/%s", root, dir->d_name);
            }
        }
    }

    closedir(dirp);

    return 0;
}





int param_parser(int argc, char *argv[])
{
    int ret;
    char *pv;
    unsigned int vint;

    //ma_intf_conf.app_name = argv[0];
    ma_intf_conf.app_name = "cfu";
    ma_intf_conf.pid = getpid();

    pv = get_arg_value_string(argc, argv, "--config");
    if (pv)
    {
        if (strlen(pv) > 127)
        {
            applog(LOG_WARNING, APP_LOG_MASK_BASE, "--config path is too long.");
        }
        else
            strcpy(g_config_path, pv);
    }
    applog(LOG_INFO, APP_LOG_MASK_BASE, "cfu use config of path: %s", g_config_path);

    ret = get_arg_value_uint(argc, argv, "--sn", &vint);
    if (ret == 0)
    {
        ma_intf_conf.sn = vint;
        applog(LOG_DEBUG, APP_LOG_MASK_BASE, "cfu sn is %d", ma_intf_conf.sn);
    }
    else
    {
        applog(LOG_INFO, APP_LOG_MASK_BASE, "cfu sn is not set or error, No connect to ma.");
    }

    return 0;
}


int get_dms_conf(void)
{
    char dump_config = 0;
    char *value = NULL;

    ConfInit();

    if (ConfYamlLoadFile(DMS_CONF_FILE) != 0) {
        /* Error already displayed. */
        exit(EXIT_FAILURE);
    }

    if (dump_config) {
        applog(LOG_DEBUG, APP_LOG_MASK_BASE, "Dump all config variable:");
        ConfDump();
    }

    if (ConfGet("ma.sguardport", &value) == 1) {
        ma_intf_conf.port_sguard = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "sguard server port: %d", ma_intf_conf.port_sguard);
    }
    if (ConfGet("ma.maport", &value) == 1) {
        ma_intf_conf.port_ma = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "ma server port: %d", ma_intf_conf.port_ma);
    }

    ConfDeInit();

    strncpy(ma_intf_conf.ip_sguard, MA_SERV_IP, 15);
    strncpy(ma_intf_conf.ip_ma, MA_SERV_IP, 15);

    return 0;
}




static int get_cfu_para(void)
{
    char *value;
    unsigned int backup_time;
    time_t time_cur;
    struct tm today;
    time_t time_nextday;
    struct tm nextday;

    if (ConfGet("local-ftp-user", &value) == 1) {
        strncpy(cfu_conf.local_ftp_user, value, 63);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "local ftp: %s", cfu_conf.local_ftp_user);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "local ftp user is not config.");
        return -1;
    }

    if (ConfGet("local-dirs.ftp", &value) == 1) {
        if (check_exist_dirs(value) != 0) {
            applog(LOG_ERR, APP_LOG_MASK_BASE, "local ftp dir %s is not exist.", value);
            return -2;
        }
        strncpy(cfu_conf.local_ftp_dir, value, DIR_LEN - 1);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "local ftp dir: %s", cfu_conf.local_ftp_dir);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "local ftp dir is not config.");
        return -2;
    }

    if (ConfGet("local-dirs.transfer", &value) == 1) {
        if (check_exist_dirs(value) != 0) {
            applog(LOG_ERR, APP_LOG_MASK_BASE, "local transfer dir %s is not exist.", value);
            return -3;
        }
        strncpy(cfu_conf.local_transfer_dir, value, DIR_LEN - 1);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "local transfer dir: %s", cfu_conf.local_transfer_dir);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "local transfer dir is not config, use default dir /transfer .");
        return -3;
    }

    if (ConfGet("local-dirs.backup", &value) == 1) {
        if (check_exist_dirs(value) != 0) {
            applog(LOG_ERR, APP_LOG_MASK_BASE, "local backup dir %s is not exist.", value);
            return -3;
        }
        strncpy(cfu_conf.local_backup_dir, value, DIR_LEN - 1);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "local backup dir: %s", cfu_conf.local_backup_dir);

        time_cur = time(NULL);
        localtime_r(&time_cur, &today);
        create_backup_dirs(cfu_conf.local_backup_dir, today.tm_year + 1900, today.tm_mon + 1, today.tm_mday);
        time_nextday = time_cur + 24 * 60 * 60;
        localtime_r(&time_nextday, &nextday);
        create_backup_dirs(cfu_conf.local_backup_dir, nextday.tm_year + 1900, nextday.tm_mon + 1, nextday.tm_mday);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "local backup dir is not config, use default dir /backup .");
        return -4;
    }

    if (ConfGet("local-backup-time", &value) == 1) {
        sscanf(value, "%u", &backup_time);
        if (backup_time >= 1 && backup_time <= 3) {
            cfu_conf.local_backup_time = backup_time;
            applog(LOG_INFO, APP_LOG_MASK_BASE, "local backup time: %d days", cfu_conf.local_backup_time);
        } else {
            cfu_conf.local_backup_time = 1;
            applog(LOG_ERR, APP_LOG_MASK_BASE, "local backup time is out of range, use default time 1 day.");
        }
    } else {
        cfu_conf.local_backup_time = 1;
        applog(LOG_ERR, APP_LOG_MASK_BASE, "local backup time is not config, use default time 1 day.");
    }

    if (ConfGet("remote-ftp.mass-ip", &value) == 1) {
        strncpy(cfu_conf.remote_mass_ftp_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote mass ftp ip: %s", cfu_conf.remote_mass_ftp_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote mass ftp ip is not config.");
        return -5;
    }

    if (ConfGet("remote-ftp.mass-user", &value) == 1) {
        strncpy(cfu_conf.remote_mass_ftp_user, value, 63);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote mass ftp user: %s", cfu_conf.remote_mass_ftp_user);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote mass ftp user is not config.");
        return -6;
    }

    if (ConfGet("remote-ftp.mass-passwd", &value) == 1) {
        strncpy(cfu_conf.remote_mass_ftp_passwd, value, 63);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote mass ftp passwd: %s", cfu_conf.remote_mass_ftp_passwd);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote mass ftp passwd is not config.");
        return -7;
    }

    if (ConfGet("remote-ftp.mass-dir", &value) == 1) {
        strncpy(cfu_conf.remote_mass_dir, value, DIR_LEN);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote mass dir: %s", cfu_conf.remote_mass_dir);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote mass dir is not config.");
        return -8;
    }

    if (ConfGet("remote-ftp.case-ip", &value) == 1) {
        strncpy(cfu_conf.remote_case_ftp_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote case ftp ip: %s", cfu_conf.remote_case_ftp_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote case ftp ip is not config.");
        return -9;
    }

    if (ConfGet("remote-ftp.case-user", &value) == 1) {
        strncpy(cfu_conf.remote_case_ftp_user, value, 63);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote case ftp user: %s", cfu_conf.remote_case_ftp_user);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote case ftp user is not config.");
        return -10;
    }

    if (ConfGet("remote-ftp.case-passwd", &value) == 1) {
        strncpy(cfu_conf.remote_case_ftp_passwd, value, 63);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote case ftp passwd: %s", cfu_conf.remote_case_ftp_passwd);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote case ftp passwd is not config.");
        return -11;
    }

    if (ConfGet("remote-ftp.case-dir", &value) == 1) {
        strncpy(cfu_conf.remote_case_dir, value, DIR_LEN);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote case dir: %s", cfu_conf.remote_case_dir);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote case dir is not config.");
        return -12;
    }

    if (ConfGet("remote-ftp.zip-ip", &value) == 1) {
        strncpy(cfu_conf.remote_zip_ftp_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote zip ftp ip: %s", cfu_conf.remote_zip_ftp_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote zip ftp ip is not config.");
        return -9;
    }

    if (ConfGet("remote-ftp.zip-user", &value) == 1) {
        strncpy(cfu_conf.remote_zip_ftp_user, value, 63);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote zip ftp user: %s", cfu_conf.remote_zip_ftp_user);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote zip ftp user is not config.");
        return -10;
    }

    if (ConfGet("remote-ftp.zip-passwd", &value) == 1) {
        strncpy(cfu_conf.remote_zip_ftp_passwd, value, 63);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote zip ftp passwd: %s", cfu_conf.remote_zip_ftp_passwd);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote zip ftp passwd is not config.");
        return -11;
    }

    if (ConfGet("remote-ftp.zip-dir", &value) == 1) {
        strncpy(cfu_conf.remote_zip_dir, value, DIR_LEN);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote zip dir: %s", cfu_conf.remote_zip_dir);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote zip dir is not config.");
        return -12;
    }

    if (ConfGet("lis.ip", &value) == 1) {
        strncpy(cfu_conf.remote_lis_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote lis ip: %s", cfu_conf.remote_lis_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote lis ip is not config.");
        return -13;
    }

    if (ConfGet("lis.port", &value) == 1) {
        cfu_conf.remote_lis_port = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "remote lis port: %d", cfu_conf.remote_lis_port);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "remote lis port is not config.");
        return -14;
    }

    if (ConfGet("rfu.ip", &value) == 1) {
        strncpy(cfu_conf.rfu_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "rfu ip: %s", cfu_conf.rfu_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "rfu ip is not config.");
        return -15;
    }

    if (ConfGet("rfu.port", &value) == 1) {
        cfu_conf.rfu_port = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "rfu port: %d", cfu_conf.rfu_port);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "rfu port is not config.");
        return -16;
    }

    if (ConfGet("online.local-ip", &value) == 1) {
        strncpy(cfu_conf.online_local_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online local ip: %s", cfu_conf.online_local_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online local ip is not config.");
        return -17;
    }

    if (ConfGet("online.local-port", &value) == 1) {
        cfu_conf.online_local_port = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online local port: %d", cfu_conf.online_local_port);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online local port is not config.");
        return -18;
    }

    if (ConfGet("online.svm-ip", &value) == 1) {
        strncpy(cfu_conf.online_svm_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online svm ip: %s", cfu_conf.online_svm_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online svm ip is not config.");
        return -19;
    }

    if (ConfGet("online.svm-port", &value) == 1) {
        cfu_conf.online_svm_port = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online svm port: %d", cfu_conf.online_svm_port);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online svm port is not config.");
        return -20;
    }

    if (ConfGet("online.mobile-ip", &value) == 1) {
        strncpy(cfu_conf.online_mobile_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online mobile ip: %s", cfu_conf.online_mobile_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online mobile ip is not config.");
        return -21;
    }

    if (ConfGet("online.mobile-port", &value) == 1) {
        cfu_conf.online_mobile_port = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online mobile port: %d", cfu_conf.online_mobile_port);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online mobile port is not config.");
        return -22;
    }

    if (ConfGet("online.voip-a-ip", &value) == 1) {
        strncpy(cfu_conf.online_voip_a_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online voip a ip: %s", cfu_conf.online_voip_a_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online voip a ip is not config.");
        return -23;
    }

    if (ConfGet("online.voip-a-port", &value) == 1) {
        cfu_conf.online_voip_a_port = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online voip a port: %d", cfu_conf.online_voip_a_port);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online voip a port is not config.");
        return -24;
    }

    if (ConfGet("online.voip-b-ip", &value) == 1) {
        strncpy(cfu_conf.online_voip_b_ip, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online voip b ip: %s", cfu_conf.online_voip_b_ip);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online voip b ip is not config.");
        return -25;
    }

    if (ConfGet("online.voip-b-port", &value) == 1) {
        cfu_conf.online_voip_b_port = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "online voip b port: %d", cfu_conf.online_voip_b_port);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "online voip b port is not config.");
        return -26;
    }

    if (ConfGet("enable.svm", &value) == 1) {
        svm_enable_flag = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "svm enable flag %d", svm_enable_flag);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "svm enable flag is not config.");
        return -27;
    }

    if (ConfGet("enable.mobile", &value) == 1) {
        mobile_enable_flag = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "mobile enable flag %d", mobile_enable_flag);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "mobile enable flag is not config.");
        return -28;
    }

    if (ConfGet("enable.voip-a", &value) == 1) {
        voip_a_enable_flag = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-a enable flag %d", voip_a_enable_flag);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "voip-a enable flag is not config.");
        return -29;
    }
    if (ConfGet("enable.voip-b", &value) == 1) {
        voip_b_enable_flag = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-b enable flag %d", voip_b_enable_flag);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "voip-b enable flag is not config.");
        return -30;
    }

    if (ConfGet("enable.lis", &value) == 1) {
        lis_enable_flag = atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "lis enable flag %d", lis_enable_flag);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "lis enable flag is not config.");
        return -31;
    }
	//获取citycode
	 if (ConfGet("citycode.src-city-code", &value) == 1) {
        strncpy(cfu_conf.src_city_code, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "citycode.src-city-code: %s", cfu_conf.src_city_code);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "citycode src-city-code is not config.");
        return -32;
    }

	 if (ConfGet("citycode.dec-city-code", &value) == 1) {
        strncpy(cfu_conf.dec_city_code, value, 15);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "citycode.dec-city-code: %s", cfu_conf.dec_city_code);
    } else {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "citycode dec-city-code is not config.");
        return -33;
    }

	//voipa话单顾虑配置
	if (ConfGet("voip-cdr-filter-enable", &value) == 1) {
        cfu_conf.voip_filter_enable =  atoi(value);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-cdr-filter-enable: %d", cfu_conf.voip_filter_enable);
    } else {
        cfu_conf.voip_filter_enable = 0;
		applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-cdr-filter-enable: %d", cfu_conf.voip_filter_enable);
    }

    if (ConfGet("voip-cdr-filter-rule", &value) == 1) {
        strncpy(cfu_conf.voip_white_list_path, value, DIR_LEN - 1);
        applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-cdr-filter-rule path: %s", cfu_conf.voip_white_list_path);
    } else {
        applog(LOG_WARNING, APP_LOG_MASK_BASE, "voip-cdr-filter-rule is not config.");
        applog(LOG_WARNING, APP_LOG_MASK_BASE, "turn-off voip-a cdr filter");
    }

    return 0;
}




int get_cfu_conf(void)
{
    char dump_config = 0;
    int ret;
    char filename[160];

    ConfInit();

    sprintf(filename,"%s/%s", g_config_path, CFU_BASE_CONF_FILE);
    if (ConfYamlLoadFile(filename) != 0) {
        /* Error already displayed. */
        exit(EXIT_FAILURE);
    }

    if (dump_config) {
        applog(LOG_DEBUG, APP_LOG_MASK_BASE, "Dump all config variable:");
        ConfDump();
    }

    memset(&cfu_conf, 0, sizeof(STRUCT_CFU_CONF_T));

    ret = get_cfu_para();
    if (ret)
    {
        applog(LOG_ERR, APP_LOG_MASK_BASE, "cfu.yaml conf is error, stop running.");
        return ret;
    }
    get_log_para();

    ConfDeInit();

    return 0;
}




#if 0

int get_local_ftpinfo(void)
{
    char *value;
    struct stat dir_stat;
    char cmd[1024];

    if (ConfGet("local-ftp-user", &value) == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_CONF, "local ftp user: %s", value);
        if (strlen(value) >= 64) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "get local ftp user error");
        } else {
            memset(ftp_info.local_user, 0, 64);
            strncpy(ftp_info.local_user, value, 63);
        }
    } else {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "no local ftp user in dcl config file");
        exit(-1);
    }

    if (ConfGet("local-ftp-dir", &value) == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_CONF, "local ftp dir: %s", value);
        if (strlen(value) >= 256) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "get local ftp dir error");
        } else {
            memset(ftp_info.local_dir, 0, 256);
            strncpy(ftp_info.local_dir, value, 255);
            if (stat(ftp_info.local_dir, &dir_stat) != 0 || (dir_stat.st_mode & S_IFMT) != S_IFDIR) {
                applog(APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_CONF, "%s is not exist or not a dir", value);
                snprintf(cmd, 1024, "mkdir -p %s", ftp_info.local_dir);
                system(cmd);
            }
            snprintf(cmd, 1024, "chown -R %s %s", ftp_info.local_user, ftp_info.local_dir);
            system(cmd);
            snprintf(cmd, 1024, "chmod -R 777 %s", ftp_info.local_dir);
            system(cmd);
        }
    } else {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "no local ftp dir in dcl config file");
        exit(-1);
    }

    return 0;
}


int get_remote_ftpinfo(void)
{
    char *value;

    if (ConfGet("remote-ftp-ip", &value) == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_CONF, "remote ftp ip: %s", value);
        if (strlen(value) >= 16) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "get remote ftp ip error");
        } else {
            memset(ftp_info.remote_ip, 0, 16);
            strncpy(ftp_info.remote_ip, value, 15);
        }
    } else {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "no remote ftp ip in dcl config file");
        exit(-1);
    }

    if (ConfGet("remote-ftp-user", &value) == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_CONF, "remote ftp user: %s", value);
        if (strlen(value) >= 64) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "get remote ftp user error");
        } else {
            memset(ftp_info.remote_user, 0, 64);
            strncpy(ftp_info.remote_user, value, 63);
        }
    } else {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "no remote ftp user in dcl config file");
        exit(-1);
    }

    if (ConfGet("remote-ftp-passwd", &value) == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_CONF, "remote ftp passwd: %s", value);
        if (strlen(value) >= 64) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "get remote ftp passwd error");
        } else {
            memset(ftp_info.remote_passwd, 0, 64);
            strncpy(ftp_info.remote_passwd, value, 63);
        }
    } else {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "no remote ftp passwd in dcl config file");
        exit(-1);
    }

    if (ConfGet("remote-ftp-dir", &value) == 1) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_CONF, "remote ftp dir: %s", value);
        if (strlen(value) >= 256) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "get remote ftp dir error");
        } else {
            memset(ftp_info.remote_dir, 0, 256);
            strncpy(ftp_info.remote_dir, value, 255);
        }
    } else {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "no remote ftp dir in dcl config file");
        exit(-1);
    }

    return 0;
}


int get_conf_info(void)
{
    char dump_config = 0;
    char filename[256] = {0};

    ConfInit();

    snprintf(filename, 255, "%s%s", dcl_conf.conf_dir, CONF_FILE);
    if (ConfYamlLoadFile(filename) != 0) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_CONF, "load svm config file %s error", filename);
        /* Error already displayed. */
        exit(EXIT_FAILURE);
    }

    if (dump_config) {
        applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_CONF, "Dump all config variable:");
        ConfDump();
    }

    get_local_ftpinfo();
    get_remote_ftpinfo();

    ConfDeInit();

    return 0;
}
#endif



