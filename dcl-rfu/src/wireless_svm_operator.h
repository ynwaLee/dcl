
#ifndef __WIRELESS_SVM_OPERATOR_H__
#define __WIRELESS_SVM_OPERATOR_H__

#include "das_operator.h"
#include "variable.h"
#include "container.h"

#define WIRELESS_SVM_SAVE_RULE_FILE_NAME  "wireless_svm_rule.xml"
#define VOIP_A_SAVE_RULE_FILE_NAME  "voip_a_rule.xml"
#define VOIP_B_SAVE_RULE_FILE_NAME  "voip_b_rule.xml"

typedef struct yj
{
    container *rule_success;        
    container *rule_fail;        
    container *rule_tmp;        
    pthread_mutex_t lock;    
    char rule_file_name[1024];
    unsigned int sys_flag;  //RESTORE_SYS_WIRELESS, RESTORE_SYS_VOIP_A, RESTORE_SYS_VOIP_B
    volatile unsigned int counter_send_fail;
    volatile unsigned int counter_send_success;
    unsigned int abnormal_clue_counter;
}yj_sys_t;

struct http_head
{
    char ac_data[1024];
    unsigned int ui_len;
};

extern yj_sys_t g_wireless;
extern yj_sys_t g_voip_a;
extern yj_sys_t g_voip_b;


extern int wireless_svm_init();
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);
extern int wireless_container_init();
extern int wireless_ismi_judge(ClueCallInfo *src);
extern int wireless_imsi_dispose(container *save_container);
extern int voip_a_dispose(container *save_container);
extern int voip_b_dispose(container *save_container);
extern int read_wireless_local_fail_rule(ClueCallInfo *pst_src);
extern int wireless_timeout(unsigned int ui_tv);
extern int wireless_read_local_success_rule();

extern unsigned int get_wireless_total_success_rule();
extern unsigned int get_wireless_total_fail_rule();

#endif


