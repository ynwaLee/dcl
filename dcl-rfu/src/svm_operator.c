
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "variable.h"
#include "svm_operator.h"
#include "wireless_svm_operator.h"
#include "ods_body.h"

extern int spu_rule_write_file_save(container **pcon_rule_array);
extern int mpu_phone_rule_write_file_save(container **pcon_rule_array);

#define mpu_phone_resend_flg 0x01
#define mpu_content_resend_flg 0x02
#define spu_resend_flg 0x04
#define vrs_resend_flg 0x08

unsigned int resend_flg = 0;

container *mpu_phone_rule = NULL;
container *mpu_phone_fail_rule = NULL;

container *mpu_content_rule = NULL;
container *mpu_content_fail_rule = NULL;

container *spu_rule = NULL;
container *spu_fail_rule = NULL;

container *vrs_rule = NULL;
container *vrs_fail_rule = NULL;

// save rule container global variable init
int svm_container_init()
{

    mpu_phone_rule = create_container();
    mpu_phone_fail_rule = create_container();

    mpu_content_rule = create_container();
    mpu_content_fail_rule = create_container();
    
    spu_rule = create_container();
    spu_fail_rule = create_container();

    vrs_rule = create_container();
    vrs_fail_rule = create_container();

    return 0;
}

//http://192.168.40.241:8080/svmnew/ps?method=uploadBlackConf&filename=rules.conf
int upload_file_to_svm(char *pc_file_path, char *pc_file_name)
{
    if( pc_file_path==NULL || pc_file_name==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed, argument error.", __FUNCTION__);
        return -1;
    }
    
    int ret = -1;
    CURL *curl;    
    CURLcode res;
    char ac_url[500] = {0};
    struct curl_httppost *formpost=NULL;  
    struct curl_httppost *lastptr=NULL;  
    struct curl_slist *headerlist=NULL;  
    static const char buf[] = "Expect:";      
    struct http_head reply_http;
    int code = -1;

    memset (&reply_http, 0, sizeof(reply_http));
    //curl_global_init (CURL_GLOBAL_ALL);

    /* Fill in the file upload field */  
     curl_formadd(&formpost,  
                   &lastptr,  
                   CURLFORM_COPYNAME, "sendfile",  
                   CURLFORM_FILE, pc_file_path,  
                   CURLFORM_END);  
 
     /* Fill in the filename field */  
     curl_formadd(&formpost,  
                   &lastptr,  
                   CURLFORM_COPYNAME, "filename",  
                   CURLFORM_COPYCONTENTS, pc_file_name,  
                   CURLFORM_END);  
      
     /* Fill in the submit field too, even if this is rarely needed */  
     curl_formadd(&formpost,  
                   &lastptr,  
                   CURLFORM_COPYNAME, "submit",  
                   CURLFORM_COPYCONTENTS, "Submit",  
                   CURLFORM_END);  

    curl =  curl_easy_init ();
    headerlist = curl_slist_append(headerlist, buf);  
    snprintf (ac_url, sizeof(ac_url), "http://%s:8080/svmnew/ps?method=uploadBlackConf&filename=%s", gst_conf.ac_svm_ser_ip, pc_file_name);
    curl_easy_setopt(curl, CURLOPT_URL, ac_url);  
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);  
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);  
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);    //设置超时
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // 定时不用信号的方式    

    //get http head
    curl_easy_setopt (curl, CURLOPT_HEADER, 1);
    curl_easy_setopt (curl, CURLOPT_HEADERDATA, &reply_http);
    curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, write_data);
    

    res = curl_easy_perform(curl);  
    
    if( res!=CURLE_OK )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "upload file to svm service failed, %s (%s)!", curl_easy_strerror(res), ac_url);
        ret = -1;
    }
    else
    {
        sscanf (reply_http.ac_data, "%*s %d %*s", &code);
        if( code!=-1 && code!=200 )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "upload file to svm service failed, (%s , %s)!", pc_file_name, reply_http.ac_data);
            ret = -1;
        }
        else if( code==200 )
        {
            applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "upload file to svm service success (%s)!", pc_file_name);
            ret = 0;
        }
        else
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "upload file to svm service success, but reply get http header of operator code failed(%s, %s)!", \
                        pc_file_name, reply_http.ac_data);
            ret = 0;
        }
    }

    curl_formfree(formpost);  
    curl_slist_free_all (headerlist);  
    curl_easy_cleanup (curl);
    //curl_global_cleanup ();
    
    return ret;
}

int rule_dispose_body(container *save_container, container *soft_con, container *soft_con_fail, int (*pf_wire_file)(container **), char *pc_file_name, unsigned int resd_flg)
{
    ClueCallInfo *pst_rule = NULL;
    ClueCallInfo *pst_old_rule = NULL;
    int flg = 0;

    //dispose old rule
    container_search(pst_rule, save_container)
    {
        if( pst_rule->CLUE_STATUS==CLUE_STATUS_DEL )
        {
            flg = 0;
            //success issue rule container
            container_search(pst_old_rule, soft_con)
            {
                if( pst_old_rule->CLUE_ID==pst_rule->CLUE_ID  )
                {
                    container_free_node (soft_con);
                    ods_free(pst_old_rule);
                    flg = 1;
                }
            }
            //issue failed rule container
            container_search(pst_old_rule, soft_con_fail)
            {
                if( pst_old_rule->CLUE_ID==pst_rule->CLUE_ID  )
                {
                    container_free_node (soft_con_fail);
                    ods_free(pst_old_rule);
                    flg = 1;
                }
            }

            if( flg==0 )
            {
                applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "a revocation of the rules, but the rules do not take effect(svm,CLUE_ID: %u, ID_TYPE: %u, CALL_ID: %s, CLUE_STATUS: %d, file: %s)", \
                            pst_rule->CLUE_ID, pst_rule->ID_TYPE, pst_rule->CALL_ID, pst_rule->CLUE_STATUS, pc_file_name);
            }
            container_free_node (save_container);
            ods_free(pst_rule);
        }
        else if( pst_rule->CLUE_STATUS!=CLUE_STATUS_ADD )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "rule CLUE_STATUS error(ID_TYPE: %u, CALL_ID: %s, CLUE_STATUS: %d, file: %s)",\
                        pst_rule->ID_TYPE, pst_rule->CALL_ID, pst_rule->CLUE_STATUS, pc_file_name);
            container_free_node (save_container);
            ods_free(pst_rule);
        }
    }
    
    // rfu.yaml file don't configure SVM restore system
    if( (gst_conf.restore_sys_flg & RESTORE_SYS_SVM)==0 )
    {
        container_catenate (soft_con_fail, save_container);
        return 0;
    }

    container *pcon_array[4] = {soft_con, soft_con_fail, save_container, 0}; 
    char ac_path[500] = {0};
    
    (*pf_wire_file)(pcon_array);

    //sendto svm
    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_svm_conf_path, pc_file_name);    
    if( upload_file_to_svm(ac_path, pc_file_name)==0 )
    {//success
        if( strcmp(pc_file_name, SVM_SPU_RULE_FILE_NAME_ISSUE )==0 )
        {
            spu_rule_write_file_save (pcon_array);        
        }
        if( strcmp(pc_file_name, SVM_MPU_PHONE_RULE_FILE_NAME_ISSUE )==0 )
        {
            mpu_phone_rule_write_file_save (pcon_array);
        }
        container_catenate (soft_con, save_container);
        //set flg, value 0
        resend_flg = resend_flg&(~resd_flg);
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "%s rule file issue success!", pc_file_name);
    }
    else
    {//fail
        pcon_array[1] = NULL;
        (*pf_wire_file)(pcon_array);
        container_catenate (soft_con_fail, save_container);
        //set flg, value 1
        resend_flg = resend_flg|resd_flg;
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "%s rule file issue failed!", pc_file_name);
    }

    return 0;
}



//<--------dispose spu rule start--------------->

int svm_spu_judge(ClueCallInfo *src)
{
    if( src==NULL )
    {
        return 0;
    }
    if( (src->ID_TYPE & ID_TYPE_PHONE_NUM)!=0 )
    {
        return 1;
    }
    return 0;
}

/* rule save to local: don't process white list*/
int spu_rule_write_file_save(container **pcon_rule_array)
{
    FILE *svm_spu_fp;
    char ac_path[255] = {0};
    char ac_data[255] = {0};
    unsigned char mode = 1;  //模式匹配所有的都要上报，
    ClueCallInfo *pst_rule = NULL;
    int i;

    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_svm_conf_path, SVM_SPU_RULE_FILE_NAME);    
    if( (svm_spu_fp=fopen(ac_path, "w+"))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call fopen(%s) function failed, %s.", ac_path, strerror(errno));
        return -1;
    }
    
    for( i=0; pcon_rule_array[i]!=NULL; ++i )
    {
        container_search(pst_rule, pcon_rule_array[i])
        {
            mode = mode|((pst_rule->METHOD)<<4);    
            snprintf (ac_data, sizeof(ac_data)-1, "%s %u %d\n", pst_rule->CALL_ID, pst_rule->CLUE_ID, mode);
            fwrite (ac_data, strlen(ac_data), 1, svm_spu_fp);    
            mode = 1;
        }
    }

    fclose (svm_spu_fp);

    return 0;
}

/* rule issue to svm: process white list*/
int spu_rule_write_file(container **pcon_rule_array)
{
    FILE *svm_spu_fp;
    char ac_path[255] = {0};
    char ac_data[255] = {0};
    unsigned char mode = 1;  //模式匹配所有的都要上报，
    ClueCallInfo *pst_rule = NULL;
    int i;

    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_svm_conf_path, SVM_SPU_RULE_FILE_NAME_ISSUE);    
    if( (svm_spu_fp=fopen(ac_path, "w+"))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call fopen(%s) function failed, %s.", ac_path, strerror(errno));
        return -1;
    }
    
    for( i=0; pcon_rule_array[i]!=NULL; ++i )
    {
        container_search(pst_rule, pcon_rule_array[i])
        {
            mode = mode|((pst_rule->METHOD)<<4);    
            if( WHITE_LIST_OBJECT_ID==pst_rule->OBJECT_ID )
            {
                snprintf (ac_data, sizeof(ac_data)-1, "%s %u %d\n", pst_rule->CALL_ID, 0, mode);
            }
            else
            {
                snprintf (ac_data, sizeof(ac_data)-1, "%s %u %d\n", pst_rule->CALL_ID, pst_rule->CLUE_ID, mode);
            }
            fwrite (ac_data, strlen(ac_data), 1, svm_spu_fp);    
            mode = 1;
        }
    }

    fclose (svm_spu_fp);

    return 0;
}

int svm_spu_dispose(container *save_container)
{
    
    rule_dispose_body (save_container, spu_rule, spu_fail_rule, spu_rule_write_file, SVM_SPU_RULE_FILE_NAME_ISSUE, spu_resend_flg);

    return 0;
}

//<--------dispose spu rule end--------------->

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//<--------dispose mpu phone rule start--------------->
int svm_mpu_phone_judge(ClueCallInfo *src)
{
    if( src==NULL )
    {
        return 0;
    }
    if( (src->ID_TYPE & ID_TYPE_PHONE_NUM)!=0 || (src->ID_TYPE & ID_TYPE_IMSI)!=0 )
    {
        return 1;
    }
    return 0;
}

/* rule save to local: don't process white list*/
int mpu_phone_rule_write_file_save(container **pcon_rule_array)
{
    FILE *svm_mpu_fp = NULL;
    char ac_path[255] = {0};
    char ac_data[255] = {0};
    unsigned char mode = 0;  
    ClueCallInfo *pst_rule_info = NULL;
    int i = 0;
    
    if( pcon_rule_array==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed,argument error!", __FUNCTION__);
        return -1;
    }

    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_svm_conf_path, SVM_MPU_PHONE_RULE_FILE_NAME);    
    if( (svm_mpu_fp=fopen(ac_path, "w+"))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call fopen(%s) function failed, %s.", ac_path, strerror(errno));
        return -1;
    }
    
    for( i=0; pcon_rule_array[i]!=NULL; ++i )
    {
        container_search(pst_rule_info, pcon_rule_array[i])
        {
            if( (pst_rule_info->ID_TYPE & ID_TYPE_PHONE_NUM)!=0 )    
            {
                if( pst_rule_info->METHOD==1 )
                {
                    mode = 16;
                }
                snprintf (ac_data, sizeof(ac_data)-1, "%s\t0\t%u\t%d\n", pst_rule_info->CALL_ID, pst_rule_info->CLUE_ID, mode);
                fwrite (ac_data, strlen(ac_data), 1, svm_mpu_fp);    
                mode = 0;
            }
            
            if( (pst_rule_info->ID_TYPE & ID_TYPE_IMSI)!=0 )    
            {
                if( pst_rule_info->METHOD==1 )
                {
                    mode = 16;
                }
                snprintf (ac_data, sizeof(ac_data)-1, "0\t%s\t%u\t%d\n", pst_rule_info->CALL_ID, pst_rule_info->CLUE_ID, mode);
                fwrite (ac_data, strlen(ac_data), 1, svm_mpu_fp);    
                mode = 0;
            }
        }
    }
    fclose (svm_mpu_fp);
    
    return 0;
}

/* rule save to local: process white list*/
int mpu_phone_rule_write_file(container **pcon_rule_array)
{
    FILE *svm_mpu_fp = NULL;
    char ac_path[255] = {0};
    char ac_data[255] = {0};
    unsigned char mode = 0;  
    ClueCallInfo *pst_rule_info = NULL;
    int i = 0;
    
    if( pcon_rule_array==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed,argument error!", __FUNCTION__);
        return -1;
    }

    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_svm_conf_path, SVM_MPU_PHONE_RULE_FILE_NAME_ISSUE);    
    if( (svm_mpu_fp=fopen(ac_path, "w+"))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call fopen(%s) function failed, %s.", ac_path, strerror(errno));
        return -1;
    }
    
    for( i=0; pcon_rule_array[i]!=NULL; ++i )
    {
        container_search(pst_rule_info, pcon_rule_array[i])
        {
            if( (pst_rule_info->ID_TYPE & ID_TYPE_PHONE_NUM)!=0 )    
            {
                if( pst_rule_info->METHOD==1 )
                {
                    mode = 16;
                }
                if( WHITE_LIST_OBJECT_ID==pst_rule_info->OBJECT_ID )
                {
                    snprintf (ac_data, sizeof(ac_data)-1, "%s\t0\t%u\t%d\n", pst_rule_info->CALL_ID, 0, mode);
                }
                else
                {
                    snprintf (ac_data, sizeof(ac_data)-1, "%s\t0\t%u\t%d\n", pst_rule_info->CALL_ID, pst_rule_info->CLUE_ID, mode);
                }
                fwrite (ac_data, strlen(ac_data), 1, svm_mpu_fp);    
                mode = 0;
            }
            
            if( (pst_rule_info->ID_TYPE & ID_TYPE_IMSI)!=0 )    
            {
                if( pst_rule_info->METHOD==1 )
                {
                    mode = 16;
                }
                if( WHITE_LIST_OBJECT_ID==pst_rule_info->OBJECT_ID )
                {
                    snprintf (ac_data, sizeof(ac_data)-1, "0\t%s\t%u\t%d\n", pst_rule_info->CALL_ID, 0, mode);
                }
                else
                {
                    snprintf (ac_data, sizeof(ac_data)-1, "0\t%s\t%u\t%d\n", pst_rule_info->CALL_ID, pst_rule_info->CLUE_ID, mode);
                }
                fwrite (ac_data, strlen(ac_data), 1, svm_mpu_fp);    
                mode = 0;
            }
        }
    }
    fclose (svm_mpu_fp);
    
    return 0;
}


int svm_mpu_phone_dispose(container *save_container)
{
    /*
        功能: 处理所有关于mpu phone的规则
        1. 处理掉取消规则的部分
            取消的规则要从已经保存下发成功或者失败的容器中删除
        2. 规则写文件
        3. 发送规则
            下发成功，所有的规则保存到下发成功的容器中
            下发失败，把本次下发的所有规则保存到下发失败的容器中
    */
    
    rule_dispose_body (save_container, mpu_phone_rule, mpu_phone_fail_rule, mpu_phone_rule_write_file, SVM_MPU_PHONE_RULE_FILE_NAME_ISSUE, mpu_phone_resend_flg);

    return 0;
}

//<--------dispose mpu phone rule end--------------->



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//<--------dispose mpu content rule start--------------->

int svm_mpu_content_judge(ClueCallInfo *src)
{
    if( src==NULL )
    {
        return 0;
    }
    if( (src->ID_TYPE & ID_TYPE_KEYWORD)!=0 )
    {
        return 1;
    }
    return 0;
}


int mpu_content_rule_write_file(container** pcon_rule_array)
{
    int i = 0, j = 0;    
    FILE *svm_mpu_fp;
    char ac_path[255] = {0};
    char ac_data[2048] = {0};
    char content[600] = {0}; 
    char *pc_tmp = NULL;
    char mode = 0;
    ClueCallInfo *pst_rule_info = NULL;
    
    if( pcon_rule_array==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed,argument error!", __FUNCTION__);
        return -1;
    }

    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_svm_conf_path, SVM_MPU_CONTENT_RULE_FILE_NAME);    
    if( (svm_mpu_fp=fopen(ac_path, "w+"))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call fopen(%s) function failed, %s.", ac_path, strerror(errno));
        return -1;
    }
    
    for( i=0; pcon_rule_array[i]!=NULL; ++i )
    {
        container_search(pst_rule_info, pcon_rule_array[i])
        {
            if( (pst_rule_info->ID_TYPE & ID_TYPE_KEYWORD)!=0 )    
            {
                pc_tmp = content;
                memset (content, 0, sizeof(content));
                j = 0;
                while( pst_rule_info->KEYWORD[j]!='\0' )
                {
                    sprintf (pc_tmp, "%02x", (unsigned char)pst_rule_info->KEYWORD[j]);
                    pc_tmp += 2;
                    ++j;    
                }
                if( pst_rule_info->METHOD==1 )
                {
                    mode = 16;
                }
                snprintf (ac_data, sizeof(ac_data)-1, "%s\t%u\t%d\n", content, pst_rule_info->CLUE_ID, mode);
                fwrite (ac_data, strlen(ac_data), 1, svm_mpu_fp);    
                mode = 0;
            }
        }
    }
    fclose (svm_mpu_fp);

    return 0;
}

int svm_mpu_content_dispose(container *save_container)
{

    rule_dispose_body (save_container, mpu_content_rule, mpu_content_fail_rule, mpu_content_rule_write_file, SVM_MPU_CONTENT_RULE_FILE_NAME, mpu_content_resend_flg);

    return 0;
}

//<--------dispose mpu content rule end--------------->


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//<--------dispose vrs rule start--------------->

int svm_vrs_judge(ClueCallInfo *src)
{
    if( src==NULL )
    {
        return 0;
    }
    if( (src->ID_TYPE & ID_TYPE_EMPHASIS)!=0 )
    {
        return 1;
    }
    return 0;
}

int vrs_rule_write_file(container **pcon_rule_array)
{
    FILE *svm_vrs_fp;
    char ac_path[255] = {0};
    char ac_data[255] = {0};
    ClueCallInfo *pst_rule_info = NULL;
    int i;

    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_svm_conf_path, SVM_VRS_RULE_FILE_NAME);    
    if( (svm_vrs_fp=fopen(ac_path, "w+"))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call fopen(%s) function failed, %s.", ac_path, strerror(errno));
        return -1;
    }
    
    for( i=0; pcon_rule_array[i]!=NULL; ++i )
    {
        container_search(pst_rule_info, pcon_rule_array[i])
        {
            //全部都要告警
            snprintf (ac_data, sizeof(ac_data)-1, "%u %u 1 %s\n", pst_rule_info->CLUE_ID, pst_rule_info->CB_ID, pst_rule_info->CALL_ID);
            fwrite (ac_data, strlen(ac_data), 1, svm_vrs_fp);    
        }
    }
    
    fclose (svm_vrs_fp);

    return 0;
}

int svm_vrs_dispose(container *save_container)
{

    rule_dispose_body (save_container, vrs_rule, vrs_fail_rule, vrs_rule_write_file, SVM_VRS_RULE_FILE_NAME, vrs_resend_flg);
    
    return 0;
}

//<--------dispose vrs rule end--------------->

ClueCallInfo* mpu_phone_string_to_struct(char *string)
{
    if( string==NULL )
    {
        return NULL;
    }

    ClueCallInfo *pst_rule = NULL;
    char ac_phone[64] = {0};
    char ac_imsi[64] = {0};
    unsigned int ui_clue_id = 0;
    int mode = 0;

    if( sscanf(string, "%s\t%s\t%u\t%d", ac_phone, ac_imsi, &ui_clue_id, &mode)==4 )
    {
        if( ac_imsi[0]=='0' )
        {//phone
            pst_rule = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));
            strncpy (pst_rule->CALL_ID, ac_phone, sizeof(pst_rule->CALL_ID)-1);
            pst_rule->CLUE_ID = ui_clue_id;
            if( mode==16 )
            {
                mode = 1;
            }
            pst_rule->METHOD = mode;
            pst_rule->CLUE_STATUS = CLUE_STATUS_ADD;
            pst_rule->ID_TYPE = ID_TYPE_PHONE_NUM;
        }
        if( ac_phone[0]=='0' )
        {//imsi
            pst_rule = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));
            strncpy (pst_rule->CALL_ID, ac_imsi, sizeof(pst_rule->CALL_ID)-1);
            pst_rule->CLUE_ID = ui_clue_id;
            if( mode==16 )
            {
                mode = 1;
            }
            pst_rule->METHOD = mode;
            pst_rule->CLUE_STATUS = CLUE_STATUS_ADD;
            pst_rule->ID_TYPE = ID_TYPE_IMSI;
        }
    }

    return pst_rule;
}

ClueCallInfo* mpu_content_string_to_struct(char *string)
{
    if( string==NULL )
    {
        return NULL;
    }

    ClueCallInfo *pst_rule = NULL;
    char ac_hex_content[1024] = {0};
    char ac_content[518] = {0};
    char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 0};
    unsigned int ui_clue_id = 0;
    int mode = 0;
    char *pc_ch = NULL;
    int i = 0, j = 0;
    int bit_flg = 0;

    if( sscanf(string, "%s\t%u\t%d", ac_hex_content, &ui_clue_id, &mode)==3 )
    {
        pc_ch = ac_hex_content;
        while( *pc_ch!='\0' )
        {
            for( i=0; i<sizeof(hex); ++i )
            {
                if( *pc_ch==hex[i] )
                {
                    break;
                }
            }
            if( i!=sizeof(hex) )
            {
                if( j>=sizeof(ac_content)-1 )
                {
                    break;
                }
                if( bit_flg==0 )
                {
                    ac_content[j] = ac_content[j]|(i<<4);
                    bit_flg = 1;
                }
                else
                {
                    ac_content[j] = ac_content[j]|i;
                    bit_flg = 0;
                    j++;
                }
            }
            pc_ch++;
        }
        ac_content[j] = '\0';

        pst_rule = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));    
        strncpy (pst_rule->KEYWORD, ac_content, sizeof(pst_rule->KEYWORD)-1);
        pst_rule->CLUE_ID = ui_clue_id;
        pst_rule->CLUE_STATUS = CLUE_STATUS_ADD;
        pst_rule->ID_TYPE = ID_TYPE_KEYWORD;
        if( mode==16 )
        {
            mode = 1;
        }
        pst_rule->METHOD = mode;

        j = 0;
        memset(ac_content, 0, sizeof(ac_content));
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "read local mcrule.conf fail, lack data option!");
    }

    return pst_rule;
}

ClueCallInfo* spu_string_to_struct(char *string)
{
    if( string==NULL )
    {
        return NULL;
    }

    ClueCallInfo *pst_rule = NULL;
    char ac_phone[64] = {0};
    unsigned int ui_clue_id = 0;
    int mode = 0;

    if( sscanf(string, "%s %u %d", ac_phone, &ui_clue_id, &mode)==3 )
    {
        pst_rule = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));            
        pst_rule->ID_TYPE = ID_TYPE_PHONE_NUM;
        pst_rule->CLUE_STATUS = CLUE_STATUS_ADD;
        
        strncpy(pst_rule->CALL_ID, ac_phone, sizeof(pst_rule->CALL_ID)-1);
        pst_rule->CLUE_ID = ui_clue_id;
        if( (mode&(1<<4))!=0 )
        {
            mode=1;
        }
        else
        {
            mode = 0;
        }
        pst_rule->METHOD = mode;
    }

    return pst_rule;
}

ClueCallInfo* vrs_string_to_struct(char *string)
{
    if( string==NULL )
    {
        return NULL;
    }

    ClueCallInfo *pst_rule = NULL;
    unsigned int ui_clue_id = 0;
    unsigned int ui_cb_id = 0;
    char call_id[64] = {0};

    if( sscanf(string, "%u %u 1 %s", &ui_clue_id, &ui_cb_id, call_id)==3 )
    {
        pst_rule = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));            
        pst_rule->ID_TYPE = ID_TYPE_EMPHASIS;
        pst_rule->CLUE_STATUS = CLUE_STATUS_ADD;

        pst_rule->CB_ID = ui_cb_id;
        pst_rule->CLUE_ID = ui_clue_id;
        strncpy (pst_rule->CALL_ID, call_id, sizeof(pst_rule->CALL_ID)-1);    
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "Read vrsrule.conf rule file error, format error(%s, right format \"clue_id cb_id 1 call_id\")!", string);
    }

    return pst_rule;
}


/*
 读取本地规则的思路
    1. 读取每个软件本地的规则
    2. 全量下来之后的处理
    3. 和本地的保存的全量对比
    4. 读取每个软件保存的本地规则
        1. 把所有读取在本地文件中读取的都认为是成功的
        2. 在全量中有，在读取成功中没有，就认为是失败的。
*/
/*
    1. 打开文件
    2. 读取数据，转换成ClueCallInfo 结构，保存到容器中
    2. 关闭文件
*/


int read_local_soft_success_rule(container *pcon_success, char *pc_file_name, ClueCallInfo *(pf_string_to_struct)(char *))
{
    char ac_path[500] = {0};
    char ac_data[1024] = {0};
    ClueCallInfo *pst_rule = NULL;
    FILE *fp = NULL;    
    
    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_conf_path_base, pc_file_name);
    if( (fp=fopen(ac_path, "r"))==NULL )
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "call fopen(%s) function failed, %s.", ac_path, strerror(errno));
        return -1;
    }
    
    while( fgets(ac_data, sizeof(ac_data), fp)!=NULL )
    {
        if( (pst_rule=(*pf_string_to_struct)(ac_data))!=NULL )        
        {
            container_add_data(pcon_success, pst_rule);
        }
        memset (ac_data, 0, sizeof(ac_data));
    }
    fclose (fp);        
    return 0;
}

int svm_read_local_success_rule()
{
    //mpu phone     
    read_local_soft_success_rule(mpu_phone_rule, SVM_MPU_PHONE_RULE_FILE_NAME, mpu_phone_string_to_struct);    
    
    //mpu content
    read_local_soft_success_rule(mpu_content_rule, SVM_MPU_CONTENT_RULE_FILE_NAME, mpu_content_string_to_struct);    
    
    //spu 
    read_local_soft_success_rule(spu_rule, SVM_SPU_RULE_FILE_NAME, spu_string_to_struct);    
    
    //vrs
    read_local_soft_success_rule(vrs_rule, SVM_VRS_RULE_FILE_NAME, vrs_string_to_struct);    

    return 0;
}


int read_svm_local_fail_rule(ClueCallInfo *pst_src)
{
    if( pst_src==NULL )
    {
        return -1;
    }

    if( svm_vrs_judge(pst_src) )
    {
        read_fail_rule(vrs_rule, vrs_fail_rule, pst_src);
        if( !container_is_null(vrs_fail_rule) )
        {
            resend_flg = resend_flg|vrs_resend_flg;
        }
    }

    if( svm_mpu_content_judge(pst_src) )
    {
        read_fail_rule(mpu_content_rule, mpu_content_fail_rule, pst_src);
        if( !container_is_null(mpu_content_fail_rule) )
        {
            resend_flg = resend_flg|mpu_content_resend_flg;
        }
    }

    if( svm_mpu_phone_judge(pst_src) )
    {
        read_fail_rule(mpu_phone_rule, mpu_phone_fail_rule, pst_src);
        if( !container_is_null(mpu_phone_fail_rule) )
        {
            resend_flg = resend_flg|mpu_phone_resend_flg;
        }
    }
    
    if( svm_spu_judge(pst_src) )
    {
        read_fail_rule(spu_rule, spu_fail_rule, pst_src);
        if( !container_is_null(spu_fail_rule) )
        {
            resend_flg = resend_flg|spu_resend_flg;
        }
    }

    return 0;
}

int resend_rule_body(container *pcon_success, container *pcon_fail, char *file_name, int (*pf_write_file)(container **), int resd_flg)
{
    char ac_path[500] = {0};
    container *pcon_array[3] = {pcon_success, pcon_fail, 0};
    
    //write rule file
    (*pf_write_file)(pcon_array);

    //sendto svm
    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_svm_conf_path, file_name);    
    if( upload_file_to_svm(ac_path, file_name)==0 )
    {//success
        if( strcmp(file_name, SVM_SPU_RULE_FILE_NAME_ISSUE )==0 )
        {
            spu_rule_write_file_save (pcon_array);        
        }
        if( strcmp(file_name, SVM_MPU_PHONE_RULE_FILE_NAME_ISSUE )==0 )
        {
            mpu_phone_rule_write_file_save (pcon_array);
        }
        container_catenate(pcon_success, pcon_fail);
        //set flg, value 0
        resend_flg = resend_flg&(~resd_flg);
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "%s phone rule issue success!", file_name);
    }
    else
    {//fail
        pcon_array[1] = NULL;
        (*pf_write_file)(pcon_array);
        //set flg, value 1
        if( container_is_null(pcon_success) && container_is_null(pcon_fail) )
        {
            resend_flg = resend_flg&(~resd_flg);
        }
        else
        {
            resend_flg = resend_flg|resd_flg;
        }
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "%s phone rule issue failed!", file_name);
    }

    return 0;    
}


int svm_timeout(unsigned int tv)
{
    // rfu.yaml file don't configure SVM restore system
    if( (gst_conf.restore_sys_flg & RESTORE_SYS_SVM)==0 )
    {
        return 0;
    }

    if( (resend_flg & mpu_phone_resend_flg)!=0 && (tv % RESEND_RULE_INTERVAL)==0 )
    {
        resend_rule_body(mpu_phone_rule, mpu_phone_fail_rule, SVM_MPU_PHONE_RULE_FILE_NAME_ISSUE, mpu_phone_rule_write_file, mpu_phone_resend_flg);
    }

    if( (resend_flg & mpu_content_resend_flg)!=0 && (tv % RESEND_RULE_INTERVAL)==0 )
    {
        resend_rule_body (mpu_content_rule, mpu_content_fail_rule, SVM_MPU_CONTENT_RULE_FILE_NAME, mpu_content_rule_write_file, mpu_content_resend_flg);
    }

    if( (resend_flg & spu_resend_flg)!=0 && (tv % RESEND_RULE_INTERVAL)==0 )
    {
        resend_rule_body (spu_rule, spu_fail_rule, SVM_SPU_RULE_FILE_NAME_ISSUE, spu_rule_write_file, spu_resend_flg);
    }

    if( (resend_flg & vrs_resend_flg)!=0 && (tv % RESEND_RULE_INTERVAL)==0 )
    {
        resend_rule_body (vrs_rule, vrs_fail_rule, SVM_VRS_RULE_FILE_NAME, vrs_rule_write_file, vrs_resend_flg);
    }
    
    return 0;
}



//get counter
unsigned int get_svm_total_success_rule()
{
    unsigned int sum = 0;
    sum = sum + container_node_num(mpu_phone_rule);
    sum = sum + container_node_num(mpu_content_rule);
    sum = sum + container_node_num(spu_rule);
    sum = sum + container_node_num(vrs_rule);
    return sum;
}

unsigned int get_svm_total_fail_rule()
{
    unsigned int sum = 0;
    sum = sum + container_node_num(mpu_phone_fail_rule);
    sum = sum + container_node_num(mpu_content_fail_rule);
    sum = sum + container_node_num(spu_fail_rule);
    sum = sum + container_node_num(vrs_fail_rule);
    return sum;
}

void svm_all_write_xml_file(void)
{
    char ac_path[500] = {0};
    container *pacon_var[] = {
        mpu_phone_rule,
        mpu_content_rule,
        vrs_rule,
        NULL
    };
    
    snprintf (ac_path, sizeof(ac_path), "%s%s", gst_conf.ac_conf_path_base, ".svm_rule.xml");        
    
    create_rule_file_array(ac_path, pacon_var);    
}






