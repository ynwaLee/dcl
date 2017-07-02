
#ifndef __DAS_OPERATOR_H__
#define __DAS_OPERATOR_H__

#include "container.h"

#define ID_TYPE_IMSI    1
#define ID_TYPE_IMEI    2
#define ID_TYPE_PHONE_NUM    4
#define ID_TYPE_KEYWORD        8
#define ID_TYPE_EMPHASIS    16

#define CLUE_STATUS_ADD 1
#define CLUE_STATUS_DEL 0
#define DAS_RULE_SAVE_FILE_NAME "das_rule.xml"
#define DAS_TMP_RULE_FILE_NAME ".das_tmp_rule.xml"
#define RESEND_RULE_INTERVAL 10 

typedef struct clue_call_info{
    unsigned int CLUE_ID;            //规则号
    unsigned char CLUE_STATUS;      //规则状态  0:无效  1：有效
    char CLUE_TYPE[32];        //规则类型CLUE_CALL
    char CALL_ID[64];          //布控号码
    char KEYWORD[256];            //关键词，短信
    unsigned int CB_ID;         //重点人员ID
    unsigned int ID_TYPE;        //布控号码类型
    unsigned int ACTION_TYPE;    //动作类型
    char LIFT_TIME[8];        //有效时间秒17280
    unsigned char METHOD;       //匹配方式    0：模糊  1：精确
    unsigned int OBJECT_ID;     //对象号        213874
    char PROTOCOL_LIST[1024];    //动态触发协议列表 
}ClueCallInfo;

extern int gi_das_data_fd;

extern int link_das_sys();
extern int das_init();
extern int das_info_dispose(fd_set *pst_rfds);
extern int operator_struct_init();
extern int create_rule_file(char *pc_file_path, container *pcon_rule);
extern int das_add_detect_fd (fd_set *rfds);
extern int das_timeout(unsigned int tv);
extern int read_rule_xml_file(char *pc_file_path, void (*pf_opt_fun)(ClueCallInfo *));
extern int open_cfu_link_port(int port);
extern int open_das_link_port(int port);
extern unsigned int get_das_total_rule();
extern int create_rule_file_array(char *pc_file_path, container **pcon_rule_array);

#endif


