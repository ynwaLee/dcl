
#ifndef __SVM_OPERATOR_H__
#define __SVM_OPERATOR_H__

#include "das_operator.h"
#include "container.h"
#include "ods_body.h"

#define SVM_SPU_RULE_FILE_NAME "rules.local"
#define SVM_SPU_RULE_FILE_NAME_ISSUE "rules.conf"
#define SVM_MPU_PHONE_RULE_FILE_NAME "mrules.local"
#define SVM_MPU_PHONE_RULE_FILE_NAME_ISSUE "mrules.conf"
#define SVM_MPU_IMSI_RULE_FILE_NAME SVM_MPU_PHONE_RULE_FILE_NAME 
#define SVM_MPU_CONTENT_RULE_FILE_NAME "mcrules.conf"
#define SVM_VRS_RULE_FILE_NAME "vrsrules.conf"
#define WHITE_LIST_OBJECT_ID 4294967196

extern int svm_rule_issue(ClueCallInfo **pst_rule_info, int rule_opt_type);
extern int svm_container_init();
extern int read_svm_local_fail_rule(ClueCallInfo *pst_src);
extern int svm_read_local_success_rule();

extern int svm_spu_judge(ClueCallInfo *src);
extern int svm_spu_dispose(container *save_container);

extern int svm_mpu_phone_judge(ClueCallInfo *src);
extern int svm_mpu_phone_dispose(container *save_container);

extern int svm_mpu_content_judge(ClueCallInfo *src);
extern int svm_mpu_content_dispose(container *save_container);

extern int svm_vrs_judge(ClueCallInfo *src);
extern int svm_vrs_dispose(container *save_container);

extern int svm_timeout(unsigned int tv);
extern unsigned int get_svm_total_success_rule();
extern unsigned int get_svm_total_fail_rule();
extern void svm_all_write_xml_file(void);

#define read_fail_rule(rule_success, rule_fail, pst_rule) \
do{\
    ClueCallInfo *pst_tmp = NULL;\
    int flg = 0;\
    container_search(pst_tmp, rule_success)\
    {\
        if( pst_tmp->CLUE_ID==pst_rule->CLUE_ID )\
        {\
            flg = 1;\
            *pst_tmp = *pst_rule;\
        }\
    }\
    if( flg==0 )\
    {\
        container_add_data(rule_fail, ods_memcpy(pst_rule));\
    }\
}while(0)


#endif


