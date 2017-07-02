
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <json/json.h>
#include <pthread.h>
#include <unistd.h>

#include "container.h"
#include "das_operator.h"
#include "svm_operator.h"
#include "wireless_svm_operator.h"
#include "ods_body.h"

yj_sys_t g_wireless;
yj_sys_t g_voip_a;
yj_sys_t g_voip_b;

//-----------------------------

int yj_tmp_add_data(yj_sys_t *sys, container *src)
{
    pthread_mutex_lock(&(sys->lock));
    container_catenate (sys->rule_tmp, src);    
    pthread_mutex_unlock(&(sys->lock));

    return 0;
}

int yj_tmp_move_data(yj_sys_t *sys, container *dest)
{
    pthread_mutex_lock(&sys->lock);
    container_catenate (dest, sys->rule_tmp);    
    pthread_mutex_unlock(&sys->lock);

    return 0;
}

int yj_tmp_is_null(yj_sys_t *sys)
{
    int ret = 0;    

    pthread_mutex_lock(&sys->lock);
    if( container_is_null(sys->rule_tmp) )
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }
    pthread_mutex_unlock(&sys->lock);

    return ret;
}

//-----------------------------


// save rule container global variable init
int wireless_container_init()
{
    g_wireless.rule_fail = create_container();
    g_wireless.rule_success = create_container();
    g_wireless.rule_tmp = create_container();
    
    g_voip_a.rule_fail = create_container();
    g_voip_a.rule_success = create_container();
    g_voip_a.rule_tmp = create_container();

    g_voip_b.rule_fail = create_container();
    g_voip_b.rule_success = create_container();
    g_voip_b.rule_tmp = create_container();

    return 0;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    struct http_head *pst_head = (struct http_head *)stream;
    strncat (pst_head->ac_data+pst_head->ui_len, ptr, sizeof(pst_head->ac_data)-pst_head->ui_len-1);
    pst_head->ui_len =  strlen(pst_head->ac_data);
    return size*nmemb;
}

int dispose_http_response(char *ret_json)
{
    if( ret_json==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed, argumet error", __FUNCTION__);
        return -1;
    }

    struct json_object *obj = NULL;
    struct json_object *obj_tmp = NULL;
    const char *pc_err = NULL;
    int ret = -1;
    
    if( (obj=json_tokener_parse(ret_json))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "YJ http response of string call json_tokener_parse function failed: %s", ret_json);
        return -1;
    }
    if( is_error(obj) )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "Yj http response of string not is json stirng: %s", ret_json);
        return -1;
    }
    if( (obj_tmp=json_object_object_get(obj, "error_message"))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "String of json of YJ http response not found \"error_message\" option, %s", ret_json);
        return -1;
    }
    pc_err = json_object_to_json_string(obj_tmp);
    if( strcmp(pc_err, "\"\"")==0 )
    {
        ret = 0;
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "YJ http response json stirng point out error: %s", pc_err);
        ret = -1;
    }

    json_object_put (obj_tmp);        
    json_object_put (obj);
    
    return ret;    
}

int send_info_to_wireless_svm(char *pc_post_data, unsigned char clue_status, struct yj_sys_conf *sys_conf)
{
    if( pc_post_data==NULL || !(clue_status==CLUE_STATUS_ADD || clue_status==CLUE_STATUS_DEL) )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed, argumet error", __FUNCTION__);
        return -1;
    }

    struct http_head http_ret;
    CURL *curl = NULL;
    char ac_url[1024] = {0};
    CURLcode res;
    int ret = -1;
    char sys_name[100] = {0};
    yj_sys_t *sys_info = NULL; 
    
    if( sys_conf==&gst_conf.wireless )
    {
        sys_info = &g_wireless;
        strcpy (sys_name, "wireless svm");
    }
    else if( sys_conf==gst_conf.voip_a ) 
    {
        strcpy (sys_name, "voip_a");
        sys_info = &g_voip_a;
    }
    else if( sys_conf==&gst_conf.voip_b )
    {
        strcpy (sys_name, "voip_b");
        sys_info = &g_voip_b;
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed, sys_conf argumet error", __FUNCTION__);
        return -1;
    }

    memset (&http_ret, 0, sizeof(struct http_head));
    if( clue_status==CLUE_STATUS_ADD )
    {
        snprintf (ac_url, sizeof(ac_url)-1, "http://%s:8080/develop/li/target/addfeature/", sys_conf->ac_server_ip);
    }
    else
    {
        snprintf (ac_url, sizeof(ac_url)-1, "http://%s:8080/develop/li/target/delfeature/", sys_conf->ac_server_ip);
    }

    //curl_global_init (CURL_GLOBAL_ALL);
    curl = curl_easy_init ();
    
    curl_easy_setopt (curl, CURLOPT_POSTFIELDS, pc_post_data);
    curl_easy_setopt (curl, CURLOPT_URL, ac_url);
    curl_easy_setopt (curl, CURLOPT_HEADER, 0);

    //get html body
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &http_ret);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);    //设置超时
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // 定时不用信号的方式    

    res = curl_easy_perform (curl);
    if( res!=CURLE_OK )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "send information to %s failed, %s(url: %s, data: %s)", \
                sys_name, curl_easy_strerror(res), ac_url, pc_post_data);
        ret = -1;
    }
    else
    {
        ret = 0;
    }

    curl_easy_cleanup (curl);
    //curl_global_cleanup ();
    
    if( ret==0 )
    {
        if( dispose_http_response(http_ret.ac_data) )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "%s http response print out rule issue failed(url: %s, data: %s)!", \
                    sys_name, ac_url, pc_post_data);
            sys_info->abnormal_clue_counter++;
            ret = -2;
        }
        else
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "send information to %s success(url: %s, data: %s)!", \
                    sys_name, ac_url, pc_post_data);
        }
    }

    return ret;
}


int rule_to_post_data(ClueCallInfo *src, char *pc_ret, unsigned int ui_len, struct yj_sys_conf *sys_conf)
{
    if(NULL == src || NULL == pc_ret || NULL == sys_conf)
    {
        return -1;
    }
    int type = -1;
    if( src->CLUE_STATUS==CLUE_STATUS_ADD )
    {
        if( (src->ID_TYPE & ID_TYPE_IMSI)!=0 )
        {
            type = 0;
            snprintf(pc_ret, ui_len-1, "password=%s&mntid=%u&feature=%s&feature_type=%d&if_voice=1&operators=%s", sys_conf->ac_pwd,\
                    src->CLUE_ID+200, src->CALL_ID, type, sys_conf->ac_operator_list);
            return 0;
        }
        else if( (src->ID_TYPE & ID_TYPE_IMEI)!=0 )
        {
            type = 2;
            snprintf(pc_ret, ui_len-1, "password=%s&mntid=%u&feature=%s&feature_type=%d&if_voice=1&operators=%s", sys_conf->ac_pwd,\
                    src->CLUE_ID+200, src->CALL_ID, type, sys_conf->ac_operator_list);
            return 0;
        }
        else if( (src->ID_TYPE & ID_TYPE_PHONE_NUM)!=0 )
        {
            type = 1;
            if( src->METHOD==0 )
            {
                snprintf(pc_ret, ui_len-1, "password=%s&mntid=%u&feature=+%s*&feature_type=%d&if_voice=1&operators=%s", sys_conf->ac_pwd,\
                        src->CLUE_ID+200, src->CALL_ID, type, sys_conf->ac_operator_list);
            }
            else
            {
                snprintf(pc_ret, ui_len-1, "password=%s&mntid=%u&feature=+%s&feature_type=%d&if_voice=1&operators=%s", sys_conf->ac_pwd,\
                        src->CLUE_ID+200, src->CALL_ID, type, sys_conf->ac_operator_list);
            }
            return 0;
        }
        else
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "in the wireless svm, there is a rule that cannot be identified"\
                        "(wireless imsi rule,ID_TYPE: %u, CALL_ID: %s, CLUE_STATUS: %d).", src->ID_TYPE, src->CALL_ID, src->CLUE_STATUS);
            return -1;
        }
    }
    else if( src->CLUE_STATUS==CLUE_STATUS_DEL )
    {
#if 1
        if( (src->ID_TYPE & ID_TYPE_IMSI)!=0 || (src->ID_TYPE & ID_TYPE_IMEI)!=0 )
        {
            snprintf(pc_ret, ui_len-1, "password=%s&feature=%s", sys_conf->ac_pwd, src->CALL_ID);
        }
        else if( (src->ID_TYPE & ID_TYPE_PHONE_NUM)!=0 )
        {
            if( src->METHOD==0 )
            {
                snprintf(pc_ret, ui_len-1, "password=%s&feature=+%s*", sys_conf->ac_pwd, src->CALL_ID);
            }
            else
            {
                snprintf(pc_ret, ui_len-1, "password=%s&feature=+%s", sys_conf->ac_pwd, src->CALL_ID);
            }
        }
        return 0;
#else
        snprintf(pc_ret, ui_len-1, "password=%s&feature=%s", sys_conf->ac_pwd, src->CALL_ID);
        return 0;
#endif
    }

    return -1;
}


/*************************************************************************************************/

int wireless_ismi_judge(ClueCallInfo *src)
{
    if( src==NULL )
    {
        return 0;
    }
    if( src->OBJECT_ID==WHITE_LIST_OBJECT_ID )
    {
        return 0;
    }
    //if( src->ID_TYPE==ID_TYPE_IMSI || src->ID_TYPE==ID_TYPE_IMEI || src->ID_TYPE==ID_TYPE_PHONE_NUM)
    if( (src->ID_TYPE & ID_TYPE_IMSI)!=0 || (src->ID_TYPE & ID_TYPE_IMEI)!=0 || (src->ID_TYPE & ID_TYPE_PHONE_NUM)!=0 )
    {
        return 1;
    }
    return 0;
}

int wireless_imsi_rule_issue(ClueCallInfo *pst_src, struct yj_sys_conf *sys_conf)
{
    char ac_post_data[1024] = {0};    
    int ret = -1;

    if( rule_to_post_data(pst_src, ac_post_data, sizeof(ac_post_data), sys_conf)==0 )
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "ac_post_data: %s !", ac_post_data);
        ret = send_info_to_wireless_svm(ac_post_data, pst_src->CLUE_STATUS, sys_conf);
    }
    
    return ret;
}

int voip_a_dispose(container *save_container)
{
    yj_tmp_add_data (&g_voip_a, save_container);    
    return 0;
}

int voip_b_dispose(container *save_container)
{
    yj_tmp_add_data (&g_voip_b, save_container);    
    return 0;
}

int wireless_imsi_dispose(container *save_container)
{
    yj_tmp_add_data (&g_wireless, save_container);    
    return 0;
}

/*************************************************************************************************/

/*
    1. 把发送成功的读取进来
    2. 在去读取发送失败的
*/

void wireless_rule_save_callback(ClueCallInfo *pst_src) 
{
    container_add_data (g_wireless.rule_success, pst_src);
}

void voip_a_rule_save_callback(ClueCallInfo *pst_src) 
{
    container_add_data (g_voip_a.rule_success, pst_src);
}

void voip_b_rule_save_callback(ClueCallInfo *pst_src) 
{
    container_add_data (g_voip_b.rule_success, pst_src);
}

int wireless_read_local_success_rule()
{
    char ac_path[500] = {0};        

    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_conf_path_base, WIRELESS_SVM_SAVE_RULE_FILE_NAME);
    read_rule_xml_file(ac_path, wireless_rule_save_callback);    

    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_conf_path_base, VOIP_A_SAVE_RULE_FILE_NAME);
    read_rule_xml_file(ac_path, voip_a_rule_save_callback);    

    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_conf_path_base, VOIP_B_SAVE_RULE_FILE_NAME);
    read_rule_xml_file(ac_path, voip_b_rule_save_callback);    

    return 0;
}

#define read_fail_rule_yj(rule_success, rule_fail, pst_rule) \
do{\
    ClueCallInfo *pst_index = NULL;\
    ClueCallInfo *pst_tmp = NULL;\
    int flg = 0;\
    container_search(pst_index, rule_success)\
    {\
        if( pst_index->CLUE_ID==pst_rule->CLUE_ID )\
        {\
            flg = 1;\
            *pst_index = *pst_rule;\
        }\
    }\
    if( flg==0 )\
    {\
        pst_tmp = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));\
        *pst_tmp = *pst_rule;\
        container_add_data(rule_fail, ods_memcpy(pst_tmp));\
    }\
}while(0)


int read_wireless_local_fail_rule(ClueCallInfo *pst_src)
{
    if( pst_src==NULL )
    {
        return -1;
    }
    
    if( wireless_ismi_judge(pst_src) )
    {
        read_fail_rule_yj(g_wireless.rule_success, g_wireless.rule_fail, pst_src);
        read_fail_rule_yj(g_voip_a.rule_success, g_voip_a.rule_fail, pst_src);
        read_fail_rule_yj(g_voip_b.rule_success, g_voip_b.rule_fail, pst_src);
    }
    
    return 0;
}

int wireless_timeout(unsigned int ui_tv)
{
    if( (g_wireless.abnormal_clue_counter>0 || g_voip_a.abnormal_clue_counter>0 || g_voip_b.abnormal_clue_counter>0) && ui_tv%10==0 )
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "YJ abnormal clue counter: wireless: %u, voip_a: %u, voip_b: %u", \
                g_wireless.abnormal_clue_counter, g_voip_a.abnormal_clue_counter, g_voip_b.abnormal_clue_counter);
    }

    return 0;
}


unsigned int get_wireless_total_success_rule()
{
    return g_wireless.counter_send_success;
}

unsigned int get_wireless_total_fail_rule()
{
    return g_wireless.counter_send_fail;
}

void* pthread_wireless(void *args)
{
    yj_sys_t *sys = (yj_sys_t *)args;
    struct yj_sys_conf *sys_conf = NULL;
    container *pcon_tmp = create_container();    
    ClueCallInfo *pst_rule = NULL;
    ClueCallInfo *pst_old_rule = NULL;
    char ac_path[500] = {0};
    int flg = 0;
    int i;
    int send_ret = 0;
	int new_sys_flag = 0;

	applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "into pthread_wireless");
    
    snprintf (ac_path, sizeof(ac_path)-1, "%s%s", gst_conf.ac_conf_path_base, sys->rule_file_name);
    if( sys==&g_wireless )        
    {
         applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "sys==&g_wireless");
        sys_conf = &(gst_conf.wireless);
    }
    else if( sys==&g_voip_a ) 
    {
         
        sys_conf = gst_conf.voip_a;
    }
    else if( sys==&g_voip_b ) 
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "sys==&voip_b");
        sys_conf = &(gst_conf.voip_b);
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "YJ pthread exit, parameter error!");
        return 0;
    }

    while( 1 )
    {
        flg = 0;
        //发送失败的规则    
        sys->counter_send_fail = container_node_num (sys->rule_fail);
        sys->counter_send_success = container_node_num (sys->rule_success);
		
        container_search(pst_rule, sys->rule_fail)
        {
            send_ret = 1;

			new_sys_flag = gst_conf.restore_sys_flg & sys->sys_flag;
			
			if(new_sys_flag == 0)
			{
			    continue;
			}

			if(sys==&g_voip_a)
			{
			    for(i = 0 ; i<gst_conf.voipa_count; i++)
			    {
			        send_ret = wireless_imsi_rule_issue(pst_rule, &gst_conf.voip_a[i]);
					if(new_sys_flag != 0 && send_ret == 0)
			        {
                        if( pst_rule->CLUE_STATUS==CLUE_STATUS_ADD )
                        {
                            container_free_node (sys->rule_fail);
                            container_add_data (sys->rule_success, pst_rule);
                            pst_rule = NULL;
                        }
                        else
                        {
                            container_search(pst_old_rule, sys->rule_success)
                            {
                                if( pst_old_rule->CLUE_ID==pst_rule->CLUE_ID )
                                {
                                    container_free_node(sys->rule_success);
                                    ods_free(pst_old_rule);
                                }
                            }
                            container_free_node(sys->rule_fail);
                            ods_free(pst_rule);
                        }
                        flg = 1;
                    }
                if( send_ret==-2 )
                {// YJ return error
                    container_free_node (sys->rule_fail);
                    ods_free(pst_rule);
                }
			    }
			}
			else
			{
			    send_ret = wireless_imsi_rule_issue(pst_rule, sys_conf);
				if(new_sys_flag != 0 && send_ret == 0)
			    {
			
                    if( pst_rule->CLUE_STATUS==CLUE_STATUS_ADD )
                    {
                        container_free_node (sys->rule_fail);
                        container_add_data (sys->rule_success, pst_rule);
                        pst_rule = NULL;
                    }
                    else
                    {
                        container_search(pst_old_rule, sys->rule_success)
                        {
                            if( pst_old_rule->CLUE_ID==pst_rule->CLUE_ID )
                            {
                                container_free_node(sys->rule_success);
                                ods_free(pst_old_rule);
                            }
                        }
                        container_free_node(sys->rule_fail);
                        ods_free(pst_rule);
                    }
                    flg = 1;
                }
                if( send_ret==-2 )
                {// YJ return error
                    container_free_node (sys->rule_fail);
                    ods_free(pst_rule);
                }
			}

			
        }
        //-----------------------------
        if( flg==1 )
        {
            create_rule_file (ac_path, sys->rule_success);
        }
        
        for( i=0; i<RESEND_RULE_INTERVAL; ++i )
        {
            sleep(1);
            if( !yj_tmp_is_null(sys) )
            {
                break;
            }
        }

        if( !yj_tmp_is_null(sys) )
        {
            yj_tmp_move_data(sys, pcon_tmp);

            container_search(pst_rule, pcon_tmp)
            {
                //对发送失败的处理
                if( pst_rule->CLUE_STATUS==CLUE_STATUS_ADD )
                {
                    container_add_data (sys->rule_fail, pst_rule);
                    pst_rule = NULL;
                }
                else if( pst_rule->CLUE_STATUS==CLUE_STATUS_DEL )
                {
                    int flg = 0;
                    container_search(pst_old_rule, sys->rule_fail)
                    {
                        if( pst_rule->CLUE_ID==pst_old_rule->CLUE_ID )
                        {
                            container_free_node(sys->rule_fail);                
                            ods_free(pst_old_rule);
                            flg = 1;
                        }
                    }
                    if( flg==0 )
                    {
                        container_add_data (sys->rule_fail, pst_rule);
                        pst_rule = NULL;
                    }
                }
                else
                {
                    applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "rule CLUE_STATUS error(ID_TYPE: %u, CALL_ID: %s, CLUE_STATUS: %d)",\
                                pst_rule->ID_TYPE, pst_rule->CALL_ID, pst_rule->CLUE_STATUS);
                }

                container_free_node (pcon_tmp);
                ods_free(pst_rule);
            }
        }
    }
    
    return NULL;
}

/*
    思路:
        1. 接到容器后，把容器内的所有内容都给一个中间容器
        2. 开线程定时查看容器是否有内容
        3. 有把容器中的内容全部取出
        4. 处理容器中的内容，把所有的当发送失败处理，并且把规则添加到发送失败的容器中
        5. 处理发送失败的规则
        6. 没处理完一条更新一个全局变量，保存他的总数(为这两个函数取值用的: get_wireless_total_success_rule, get_wireless_total_fail_rule)
*/

//这个初始化一定要等到读完本地规则之后
int wireless_svm_init()
{
    pthread_t pth;
    
    g_wireless.sys_flag = RESTORE_SYS_WIRELESS;
    strncpy (g_wireless.rule_file_name, WIRELESS_SVM_SAVE_RULE_FILE_NAME, sizeof(g_wireless.rule_file_name)-1);
    pthread_mutex_init (&g_wireless.lock, NULL);
    pthread_create (&pth, NULL, pthread_wireless, (void *)&g_wireless);
    pthread_detach (pth);
    
    g_voip_a.sys_flag = RESTORE_SYS_VOIP_A;
    strncpy (g_voip_a.rule_file_name, VOIP_A_SAVE_RULE_FILE_NAME, sizeof(g_voip_a.rule_file_name)-1);
    pthread_mutex_init (&g_voip_a.lock, NULL);
    pthread_create (&pth, NULL, pthread_wireless, (void *)&g_voip_a);
    pthread_detach (pth);

    g_voip_b.sys_flag = RESTORE_SYS_VOIP_B;
    strncpy (g_voip_b.rule_file_name, VOIP_B_SAVE_RULE_FILE_NAME, sizeof(g_voip_b.rule_file_name)-1);
    pthread_mutex_init (&g_voip_b.lock, NULL);
    pthread_create (&pth, NULL, pthread_wireless, (void *)&g_voip_b);
    pthread_detach (pth);

    return 0;
}



