
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "variable.h"

#define DMS_CONFIG_FILE_PATH "/usr/local/etc/dms.yaml"

extern int read_config();
extern int check_directory(char *pc_dir);
extern int read_rfu_config(char *pc_file_dir, struct conf *pst_conf);
extern int read_dms_config(char *pc_file_path, struct conf *pst_conf);

#endif

