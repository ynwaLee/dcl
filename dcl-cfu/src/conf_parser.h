#ifndef __CONF_PARSER_H__
#define __CONF_PARSER_H__


#define DIR_LEN     256

#define DMS_CONF_FILE       "/usr/local/etc/dms.yaml"
#define CFU_BASE_CONF_FILE  "cfu.yaml"

#define DEFAULT_CONFIG_PATH "/usr/local/etc/cfu"

#define MASS_DIR    "mass"
#define CASE_DIR    "case"
#define ZIP_DIR    "zip"

extern char g_config_path[DIR_LEN];


typedef struct STRUCT_CFU_CONF_ {
    char local_ftp_user[64];
    char local_ftp_dir[DIR_LEN];
    char local_transfer_dir[DIR_LEN];
    char local_backup_dir[DIR_LEN];
    unsigned int local_backup_time;
    char remote_mass_ftp_ip[16];
    char remote_mass_ftp_user[64];
    char remote_mass_ftp_passwd[64];
    char remote_mass_dir[DIR_LEN];
    char remote_case_ftp_ip[16];
    char remote_case_ftp_user[64];
    char remote_case_ftp_passwd[64];
    char remote_case_dir[DIR_LEN];
    char remote_zip_ftp_ip[16];
    char remote_zip_ftp_user[64];
    char remote_zip_ftp_passwd[64];
    char remote_zip_dir[DIR_LEN];
    char remote_lis_ip[16];
    unsigned int remote_lis_port;
    char rfu_ip[16];
    unsigned int rfu_port;
    char online_local_ip[16];
    unsigned int online_local_port;
    char online_svm_ip[16];
    unsigned int online_svm_port;
    char online_mobile_ip[16];
    unsigned int online_mobile_port;
    char online_voip_a_ip[16];
    unsigned int online_voip_a_port;
    char online_voip_b_ip[16];
    unsigned int online_voip_b_port;
	char src_city_code[16];
	char dec_city_code[16];
	int voip_filter_enable;
	char voip_white_list_path[DIR_LEN];
} STRUCT_CFU_CONF_T;


extern STRUCT_CFU_CONF_T cfu_conf;


extern int svm_enable_flag;
extern int mobile_enable_flag;
extern int voip_a_enable_flag;
extern int voip_b_enable_flag;
extern int lis_enable_flag;

int param_parser(int argc, char *argv[]);
int get_dms_conf(void);
int get_cfu_conf(void);

int create_backup_dirs(char *root, int year, int month, int day);
int cleanup_backup_dirs(char *root);


int check_transfer_dir(void);

int check_exist_dirs(char *path);


#endif
