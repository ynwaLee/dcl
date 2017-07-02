

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "../../common/conf.h"
#include "variable.h"
#include "config.h"

struct conf gst_conf;



int check_ip_addr(char *pc_ip)
{
    if( pc_ip==NULL || strlen(pc_ip)==0 || strlen(pc_ip)>16 )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function fail, argument error!", __FUNCTION__);    
        return -1;
    }

    int a,b,c,d;
    
    a=b=c=d=-1;
    if( sscanf(pc_ip, "%d.%d.%d.%d", &a, &b, &c, &d)==4 )
    {
        if( (a>=0 && a<=255) && (b>=0 && b<=255) && (c>=0 && c<=255) && (d>=0 && d<=255) )
        {
            return 0;
        }
    }

    return -1;
}

int check_directory(char *pc_dir)
{
    struct stat file_stat;
    int ret = -1;

    if( pc_dir==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function fail, argument error!", __FUNCTION__);    
        return -1;
    }
    
    if( strlen(pc_dir)==0 )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "path value is null");    
        return -1;
    }
    
    if( (ret=stat(pc_dir, &file_stat)) )
    {
        if( errno==ENOENT )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "%s specifies a file does not exist.", pc_dir);    
            return -1;
        }
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "in the %s function, call stat function fail, %s.", __FUNCTION__, strerror(errno));    
        return -1;
    }
    
    if( !(S_ISDIR(file_stat.st_mode)) )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "%s not a directory.", pc_dir);    
        return -1;
    }
    
    if( pc_dir[strlen(pc_dir)-1]!='/' )
    {
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "%s rogram automatically fills '/'.", pc_dir);    
        pc_dir[strlen(pc_dir)] = '/';
        pc_dir[strlen(pc_dir)+1] = '\0';
    }
    
    return 0;
}


void delete_stirng_comma(char *src, int src_len)
{
    char *tmp = (char *)MY_MALLOC(src_len);
    char *start_tmp = tmp;
    char *ptr = src;
    
    while( *ptr!='\0' && ptr-src<src_len )        
    {
        if( *ptr!=',' )
        {
            *tmp = *ptr;
            tmp++;
        }
        ptr++;    
    }
    *tmp = '\0';    

    memset (src, 0, src_len);
    strcpy (src, start_tmp);
    MY_FREE(start_tmp);
}


void read_yj_sys_info(char *pc_file_path, struct conf *pst_conf)
{
    char *pc_value = NULL;
    
    // start wireless information
    if( ConfGet("wireless-svm.server-ip", &pc_value) )
    {    
        if( check_ip_addr(pc_value) )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "wireless-svm.server-ip get error(%s), format error, exit proccess", pc_value);
            exit (-1);
        }
        memset (pst_conf->wireless.ac_server_ip, 0, sizeof(pst_conf->wireless.ac_server_ip));
        strncpy (pst_conf->wireless.ac_server_ip, pc_value, sizeof(pst_conf->wireless.ac_server_ip)-1);    
    }
    else
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found wireless-svm.server-ip option, exit proccess ", pc_file_path);
        exit (-1);
    }
    
    if( ConfGet("wireless-svm.system-pwd", &pc_value) )
    {    
        if( strlen(pc_value)>=sizeof(pst_conf->wireless.ac_pwd) )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "wireless-svm.system-pwd get error(%s), the data is too long, exit proccess", pc_value);
            exit (-1);
        }
        memset (pst_conf->wireless.ac_pwd, 0, sizeof(pst_conf->wireless.ac_pwd));
        strncpy (pst_conf->wireless.ac_pwd, pc_value, sizeof(pst_conf->wireless.ac_pwd)-1);    
    }
    else
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found wireless-svm.system-pwd option, exit proccess ", pc_file_path);
        exit (-1);
    }
    
    if( ConfGet("wireless-svm.operator-list", &pc_value) )
    {
        strncpy (pst_conf->wireless.ac_operator_list, pc_value, sizeof(pst_conf->wireless.ac_operator_list)-1);    
        delete_stirng_comma (pst_conf->wireless.ac_operator_list, (int)sizeof(pst_conf->wireless.ac_operator_list));
    }
    else
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found wireless-svm.operator-list option, exit proccess ", pc_file_path);
        exit (-1);
#if 0
        ConfNode *base;
        ConfNode *child;
        unsigned int len = 0;
        int tmp;
        TAILQ_FOREACH(child, &base->head, next)
        {
            tmp = atoi (child->val);
            if( tmp>0 && tmp<10 )
            {
                strncat (pst_conf->ac_operator_list+len, child->val, sizeof(pst_conf->ac_operator_list)-len-0);
                len = strlen(pst_conf->ac_operator_list);
            }
            else
            {
                applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file, there is an operator serial number illegal(%s)", ac_file_path, child->val);
            }
        }
#endif
    }
    
    // start voip a information
    ConfNode *base;
	ConfNode *child;
	unsigned int id = 0;
	char tmp_voipa_ip[64] = {0};
	char tmp_pwd[128] = {0};
	char tmp_operator_list[128] = {0};
	pst_conf->voipa_count = 0;

	base = ConfGetNode("voip-a");
	if (base == NULL)
	{
	    applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "voip-a get error, format error, exit proccess");
		exit (-1);
	}
	
	TAILQ_FOREACH(child, &base->head, next)
	{
	    if (!strcmp(child->val, "id"))
		{
		    ConfNode *subchild;
			TAILQ_FOREACH(subchild, &child->head, next)
			{
			    if ((!strcmp(subchild->name, "id")))
				{
				    if ((subchild->val == NULL)||(subchild->val[0] == 0))
			            break;
				    sscanf(subchild->val, "%u", &id);
				    if (id > MAX_VOIPA_NUM)
				    {
				        applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-a id is too big, this id %u will be ignored.", id);
					    break;
				    }
				    if (id == 0)
				    {
				        applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-a id == 0, this id %u will be ignored.", id);
					    continue;
				    }
						
			    }
				//获取ip
				if ((!strcmp(subchild->name, "server-ip")))
				{
			        if ((subchild->val == NULL)||(subchild->val[0] == 0))
					{
					    applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-a id = %d has no server-ip!", id);
					    exit(-1);
					}
					memset(tmp_voipa_ip , 0 , 64);
					strcpy(tmp_voipa_ip , subchild->val);
					if( check_ip_addr(tmp_voipa_ip) )
                    {
                        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "voip-a.server-ip get error(%s), format error, exit proccess", tmp_voipa_ip);
                        exit (-1);
                    }
					memset(pst_conf->voip_a[pst_conf->voipa_count].ac_server_ip , 0 , sizeof(pst_conf->voip_a[0].ac_server_ip));
					strncpy (pst_conf->voip_a[pst_conf->voipa_count].ac_server_ip, tmp_voipa_ip, sizeof(pst_conf->voip_a[0].ac_server_ip)-1);	
				}
				//获取密码
				if ((!strcmp(subchild->name, "system-pwd")))
				{
			        if ((subchild->val == NULL))
					{
					    applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-a id = %d has no system-pwd!", id);
					    exit(-1);
					}
					memset(tmp_pwd , 0 , 128);
					strcpy(tmp_pwd , subchild->val);

					memset(pst_conf->voip_a[pst_conf->voipa_count].ac_pwd , 0 , sizeof(pst_conf->voip_a[0].ac_pwd));
					strncpy (pst_conf->voip_a[pst_conf->voipa_count].ac_pwd, tmp_pwd, sizeof(pst_conf->voip_a[0].ac_pwd)-1);	
				}
				//获取操作列表
				if ((!strcmp(subchild->name, "operator-list")))
				{
			        if ((subchild->val == NULL))
					{
					    applog(LOG_INFO, APP_LOG_MASK_BASE, "voip-a id = %d has no operator-list!", id);
					    exit(-1);
					}
					memset(tmp_operator_list , 0 , 128);
					strcpy(tmp_operator_list , subchild->val);

					memset(pst_conf->voip_a[pst_conf->voipa_count].ac_operator_list , 0 , sizeof(pst_conf->voip_a[0].ac_operator_list));
					strncpy (pst_conf->voip_a[pst_conf->voipa_count].ac_operator_list, tmp_operator_list, sizeof(pst_conf->voip_a[0].ac_operator_list)-1);	

                    delete_stirng_comma (pst_conf->voip_a[pst_conf->voipa_count].ac_operator_list, (int)sizeof(pst_conf->voip_a[0].ac_operator_list));

					pst_conf->voipa_count++;
				}
			
		    }
	    }
    }
		
	
	
    #if 0
    if( ConfGet("voip-a.server-ip", &pc_value) )
    {    
        if( check_ip_addr(pc_value) )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "voip-a.server-ip get error(%s), format error, exit proccess", pc_value);
            exit (-1);
        }
        memset (pst_conf->voip_a.ac_server_ip, 0, sizeof(pst_conf->voip_a.ac_server_ip));
        strncpy (pst_conf->voip_a.ac_server_ip, pc_value, sizeof(pst_conf->voip_a.ac_server_ip)-1);    
    }
    else
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found voip-a.server-ip option, exit proccess ", pc_file_path);
        exit (-1);
    }
    
    if( ConfGet("voip-a.system-pwd", &pc_value) )
    {    
        if( strlen(pc_value)>=sizeof(pst_conf->voip_a.ac_pwd) )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "voip-a.system-pwd get error(%s), the data is too long, exit proccess", pc_value);
            exit (-1);
        }
        memset (pst_conf->voip_a.ac_pwd, 0, sizeof(pst_conf->voip_a.ac_pwd));
        strncpy (pst_conf->voip_a.ac_pwd, pc_value, sizeof(pst_conf->voip_a.ac_pwd)-1);    
    }
    else
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found voip-a.system-pwd option, exit proccess ", pc_file_path);
        exit (-1);
    }
    
    if( ConfGet("voip-a.operator-list", &pc_value) )
    {
        strncpy (pst_conf->voip_a.ac_operator_list, pc_value, sizeof(pst_conf->voip_a.ac_operator_list)-1);    
        delete_stirng_comma (pst_conf->voip_a.ac_operator_list, (int)sizeof(pst_conf->voip_a.ac_operator_list));
    }
    else
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found voip-a.operator-list option, exit proccess ", pc_file_path);
        exit (-1);
    }
	#endif

    // start voip_b    
    if( ConfGet("voip-b.server-ip", &pc_value) )
    {    
        if( check_ip_addr(pc_value) )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "voip-b.server-ip get error(%s), format error, exit proccess", pc_value);
            exit (-1);
        }
        memset (pst_conf->voip_b.ac_server_ip, 0, sizeof(pst_conf->voip_b.ac_server_ip));
        strncpy (pst_conf->voip_b.ac_server_ip, pc_value, sizeof(pst_conf->voip_b.ac_server_ip)-1);    
    }
    else
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found voip-b.server-ip option, exit proccess ", pc_file_path);
        exit (-1);
    }
    
    if( ConfGet("voip-b.system-pwd", &pc_value) )
    {    
        if( strlen(pc_value)>=sizeof(pst_conf->voip_b.ac_pwd) )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "voip-b.system-pwd get error(%s), the data is too long, exit proccess", pc_value);
            exit (-1);
        }
        memset (pst_conf->voip_b.ac_pwd, 0, sizeof(pst_conf->voip_b.ac_pwd));
        strncpy (pst_conf->voip_b.ac_pwd, pc_value, sizeof(pst_conf->voip_b.ac_pwd)-1);    
    }
    else
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found voip-b.system-pwd option, exit proccess ", pc_file_path);
        exit (-1);
    }
    
    if( ConfGet("voip-b.operator-list", &pc_value) )
    {
        strncpy (pst_conf->voip_b.ac_operator_list, pc_value, sizeof(pst_conf->voip_b.ac_operator_list)-1);    
        delete_stirng_comma (pst_conf->voip_b.ac_operator_list, (int)sizeof(pst_conf->voip_b.ac_operator_list));
    }
    else
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found voip-b.operator-list option, exit proccess ", pc_file_path);
        exit (-1);
    }

}

int read_rfu_config(char *pc_file_dir, struct conf *pst_conf)
{
    char ac_file_path[500] = {0};
    char *pc_value = NULL;
    char ac_restore_sys[500] = {0};
	int k = 0;

    if( pc_file_dir==NULL || pst_conf==NULL )    
    {
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "call %s function fail, argument error!", __FUNCTION__);    
        exit (-1);
    }

    //default value
    pst_conf->das_data_port = 59999;
    pst_conf->das_signal_port = 59996;
    pst_conf->cfu_listen_port = 2100;
    applog_set_debug_mask (0);
    int tmp = pst_conf->if_link_ma_flg;
    memset (pst_conf, 0, sizeof(struct conf));
    pst_conf->if_link_ma_flg = tmp;

    ConfInit ();
    snprintf(ac_file_path, sizeof(ac_file_path), "%s%s", pc_file_dir, CONFIG_RFU);
    if( ConfYamlLoadFile(ac_file_path)==0 )
    {
        if( ConfGet("das.signal-port", &pc_value) )
        {
            pst_conf->das_signal_port = atoi (pc_value);    
            if( pst_conf->das_signal_port<1024 || pst_conf->das_signal_port>65535 )
            {
                applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "das.signal_port get error(%s), use default value 59996", pc_value);
                pst_conf->das_signal_port = 59996;
            }
        }
        else
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "%s config file not found das.signal_port option!", ac_file_path);
        }

        if( ConfGet("das.data-port", &pc_value) )
        {
            pst_conf->das_data_port = atoi (pc_value);    
            if( pst_conf->das_signal_port<1024 ||  pst_conf->das_signal_port>65535 )
            {
                applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "das.data_port get error(%s), use default value 59999", pc_value);
                pst_conf->das_data_port = 59999;
            }
        }
        else
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "%s config file not found das.data_port option!", ac_file_path);
        }

        if( ConfGet("cfu-open.port", &pc_value) )
        {
            pst_conf->cfu_listen_port = atoi (pc_value);    
            if( pst_conf->cfu_listen_port<1024 ||  pst_conf->cfu_listen_port>65535 )
            {
                applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "cfu-open.port get error(%s), use default value 2100", pc_value);
                pst_conf->cfu_listen_port = 2100;
            }
        }
        else
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "%s config file not found cfu-open.port option!", ac_file_path);
        }


        if( ConfGet("das.ip", &pc_value) )
        {    
            if( check_ip_addr(pc_value) )
            {
                applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "das.ip get error(%s), format error, exit proccess", pc_value);
                exit (-1);
            }
            memset (pst_conf->ac_das_ip, 0, sizeof(pst_conf->ac_das_ip));
            strncpy (pst_conf->ac_das_ip, pc_value, sizeof(pst_conf->ac_das_ip)-1);    
        }
        else
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found das.ip option, exit proccess ", ac_file_path);
            exit (-1);
        }
        
        if( ConfGet("path.save-file-directory", &pc_value) )
        {    
            if( strlen(pc_value)>sizeof(pst_conf->ac_svm_conf_path)-55 )
            {
                applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "path.save-file-directory option of value too long(%s), exit proccess", pc_value);
                exit (-1);
            }
            strncpy (pst_conf->ac_conf_path_base, pc_value, sizeof(pst_conf->ac_conf_path_base)-2);    
            if( check_directory(pst_conf->ac_conf_path_base) )
            {
                applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "path.save-file-directory option of value error, exit proccess");
                exit (-1);
            }
        }
        else
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found path.save-file-directory option, exit proccess ", ac_file_path);
            exit (-1);
        }
        
        if( ConfGet("svm.server-ip", &pc_value) )
        {    
            if( check_ip_addr(pc_value) )
            {
                applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "svm.server-ip get error(%s), format error, exit proccess", pc_value);
                exit (-1);
            }
            memset (pst_conf->ac_svm_ser_ip, 0, sizeof(pst_conf->ac_svm_ser_ip));
            strncpy (pst_conf->ac_svm_ser_ip, pc_value, sizeof(pst_conf->ac_svm_ser_ip)-1);    
        }
        else
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file not found svm.server-ip option, exit proccess ", ac_file_path);
            exit (-1);
        }
        
        read_yj_sys_info(ac_file_path, pst_conf);
        
        if( ConfGet("restore-system", &pc_value) )
        {
            char *pc_buffer = MY_MALLOC(strlen(pc_value)+1);
            char *pac_name[100] = {0};
            char *pc_src = NULL;
            char *pc_tmp = NULL;
            int i = 0, j = 0;
            strcpy (pc_buffer, pc_value);
            pc_src = pc_buffer;    
            while( (pc_tmp=strchr(pc_src, ','))!=NULL )
            {
                *pc_tmp = '\0';
                pac_name[i++] = pc_src;
                pc_src = pc_tmp+1;
                if( i==98 )
                {
                  break;
                }
            }
            pac_name[i++] = pc_src;
            for( j=0; j<i; ++j )
            {
                if( strcmp(pac_name[j], "svm")==0 )    
                {
                    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "restore_sys_flg | 1(SVM)");
                    pst_conf->restore_sys_flg |= RESTORE_SYS_SVM;
                    strcat (ac_restore_sys, "svm ");    
                }
                if( strcmp(pac_name[j], "wireless_svm")==0 )    
                {
                    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "restore_sys_flg | 2(wireless)");
                    pst_conf->restore_sys_flg |= RESTORE_SYS_WIRELESS;
                    strcat (ac_restore_sys, "wireless_svm ");    
                }
                if( strcmp(pac_name[j], "voip_a")==0 )    
                {
                    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "restore_sys_flg | 4(voipa)");
                    pst_conf->restore_sys_flg |= RESTORE_SYS_VOIP_A;
                    strcat (ac_restore_sys, "voip_a ");    
                }
                if( strcmp(pac_name[j], "voip_b")==0 )    
                {
                    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "restore_sys_flg | 8(voipb)");
                    pst_conf->restore_sys_flg |= RESTORE_SYS_VOIP_B;
                    strcat (ac_restore_sys, "voip_b ");    
                }
            }
            MY_FREE(pc_buffer);
        }
        else
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "%s config file, not found restore-sys option, exit proccess!", ac_file_path);
            exit (-1);
        }
        
        get_log_para ();
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "read config failed, load file(%s) error! exit proccess", ac_file_path);
        exit (-1);
    }
            
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "rfu.yaml config file information");
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "das signal port: %d", pst_conf->das_signal_port);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "das data port: %d", pst_conf->das_data_port);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "cfu link port: %d", pst_conf->cfu_listen_port);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "das ip: %s", pst_conf->ac_das_ip);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "svm web service ip: %s", pst_conf->ac_svm_ser_ip);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "save conf save path: %s", pst_conf->ac_conf_path_base);
    // yj wireless
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "wireless svm service ip: %s", pst_conf->wireless.ac_server_ip);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "wireless svm system passwd: %s", pst_conf->wireless.ac_pwd);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "wireless svm oprator list: %s", pst_conf->wireless.ac_operator_list);
    // yj voip a    

	for(k = 0 ; k<pst_conf->voipa_count; k++)
	{
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "voip-a svm service ip: %s", pst_conf->voip_a[k].ac_server_ip);
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "voip-a svm system passwd: %s", pst_conf->voip_a[k].ac_pwd);
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "voip-a svm oprator list: %s", pst_conf->voip_a[k].ac_operator_list);
	}
	// yj voip b
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "voip-b svm service ip: %s", pst_conf->voip_b.ac_server_ip);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "voip-b svm system passwd: %s", pst_conf->voip_b.ac_pwd);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "voip-b svm oprator list: %s", pst_conf->voip_b.ac_operator_list);

    strcpy (pst_conf->ac_svm_conf_path, pst_conf->ac_conf_path_base);    
    if( strlen(ac_restore_sys)==0 )                
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "The rfu.yaml doesn't configure restore system(%02x).", pst_conf->restore_sys_flg);
    }
    else
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "restore system: %s(%02x).", ac_restore_sys, pst_conf->restore_sys_flg);
    }

    ConfDeInit ();
    
    return 0;    
}


int read_dms_config(char *pc_file_path, struct conf *pst_conf)
{
    if( pc_file_path==NULL || pst_conf==NULL )    
    {
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "call %s function fail, argument error!", __FUNCTION__);    
        exit (-1);
    }    
    
    if( gst_conf.if_link_ma_flg==0 )
    {
        return -1;
    }

    char *pc_value = NULL;
    //use value
    pst_conf->ma_port = 2001;    
    pst_conf->sguard_port = 2002;
    pst_conf->if_link_ma_flg = 1;

    ConfInit ();
    if( ConfYamlLoadFile(pc_file_path)==0 )
    {
        if( ConfGet("ma.maport", &pc_value) )
        {
            pst_conf->ma_port = atoi (pc_value);    
            if( pst_conf->ma_port<1024 || pst_conf->ma_port>65535 )
            {
                applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "ma.maport get error(%s), do not link ma", pc_value);
                pst_conf->if_link_ma_flg = 0;
            }
        }
        else
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "%s config file not found ma.maport option, use default value: 2001", pc_file_path);
            pst_conf->ma_port = 2001;
        }

        if( ConfGet("ma.sguardport", &pc_value) )
        {
            pst_conf->sguard_port = atoi (pc_value);    
            if( pst_conf->sguard_port<1024 || pst_conf->sguard_port>65535 )
            {
                applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "ma.sguardport get error(%s), do not link ma", pc_value);
                pst_conf->if_link_ma_flg = 0;
            }
        }
        else
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "%s config file not found ma.sguardport option, use default value: 2000", pc_file_path);
            pst_conf->sguard_port = 2000;
        }

    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "read config failed, load file(%s) error! do not link ma", pc_file_path);
        pst_conf->if_link_ma_flg = 0;
    }
    ConfDeInit ();
    
    //show
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "dms.yaml config file information");
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "ma listen port: %d", pst_conf->ma_port);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "sguard listen port: %d", pst_conf->sguard_port);
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "whether to link ma: %s", pst_conf->if_link_ma_flg?"yes":"no");

    return 0;
}


int read_config()
{
    read_rfu_config (gac_config_path, &gst_conf);    

    read_dms_config (DMS_CONFIG_FILE_PATH, &gst_conf);
    
    return 0;
}




