
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "../../common/applog.h"
#include "../../common/ma_pkt.h"
#include "../../common/sock_cli.h"
#include "variable.h"
#include "ma_operator.h"
#include "das_operator.h"
#include "type_macro.h"
#include "config.h"
#include "svm_operator.h"
#include "wireless_svm_operator.h"


int gi_ma_fd = -1;
int gi_sguard_fd = -1;

int sendto_info(int fd, void *pv_data, unsigned int ui_len)
{
    if( pv_data==NULL || ui_len==0 || fd<0 )
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "call %s function failed, argument error!", __FUNCTION__);
        return -1;
    }
    int ret = -1;
    
    if( (ret=send(fd, pv_data, ui_len, MSG_NOSIGNAL))<=0 )
    {
        if( fd==gi_ma_fd )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call send(to ma) function failed, %s!", ret<0?strerror(errno):"send return 0");
            MY_CLOSE(gi_ma_fd);
        }
        else if( fd==gi_sguard_fd )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call send(to sguard) function failed, %s!", ret<0?strerror(errno):"send return 0");
            MY_CLOSE(gi_sguard_fd);
        }
        else
        {
            MY_CLOSE(fd);
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call send function failed, %s!", ret<0?strerror(errno):"send return 0");
        }
    }

    return ret;
}

#if 0
//define in the common/ma_pkt.h
struct pkt_header {
    uint8_t ver;
    uint8_t reply;
    uint16_t cmd;
    uint16_t num;
    uint16_t len;
}
#endif

#define unpack_head(pkt_h) \
do{\
    pkt_h->cmd = ntohs(pkt_h->cmd);\
    pkt_h->num = ntohs(pkt_h->num);\
    pkt_h->len = ntohs(pkt_h->len);\
}while(0)

#define pkt_head_length (sizeof(struct pkt_header))

int read_ma_data(struct pkt_header *pst_head, unsigned char **ppc_ret)
{
    struct pkt_header *pst_head_tmp = NULL;
    unsigned char ac_head[pkt_head_length] = {0};        
    unsigned char *pc_data = NULL;

    if( recv(gi_ma_fd, ac_head, pkt_head_length, 0)<=0 )
    {
        MY_CLOSE(gi_ma_fd);
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call recv function failed, %s!", strerror(errno));
        return -1;
    }
    
    pst_head_tmp = (struct pkt_header *)(ac_head);
    unpack_head(pst_head_tmp);
    *pst_head = *pst_head_tmp;
    if( pst_head->len==pkt_head_length )
    {
        return 0;
    }

    pc_data = (unsigned char *)MY_MALLOC(pst_head_tmp->len);
    if( recv(gi_ma_fd, pc_data, pst_head_tmp->len, 0)<=0 )
    {
        MY_CLOSE(gi_ma_fd);
        MY_FREE(pc_data);
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call recv function failed, %s!", strerror(errno));
        return -1;
    }
    *ppc_ret = pc_data;

    return 0;
}

int ma_reload_conf_file(unsigned char *pc_src, unsigned int ui_len)
{
    struct conf st_conf_tmp = gst_conf;
    char *apc_conf_name[10] = {0};
    int sum = 0;
    int i;

    if( (sum=get_ma_reload_conf(pc_src, ui_len, apc_conf_name, 10))<=0 )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "in the reload config packet not found config file name");
        return -1;
    }
    
    for( i=0; i<sum; ++i )
    {
        if( strcmp(apc_conf_name[i], "rfu.yaml")==0 )
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "ma notice reload rfu.yaml config!");
            read_rfu_config (gac_config_path, &gst_conf);    
            if( st_conf_tmp.das_data_port!=gst_conf.das_data_port )
            {
                applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "alter rfu listen das link port: %d", gst_conf.das_data_port);
                open_das_link_port (gst_conf.das_data_port);
            }
            if( st_conf_tmp.das_signal_port!=gst_conf.das_signal_port || \
                strcmp(gst_conf.ac_das_ip, st_conf_tmp.ac_das_ip)!=0 )
            {
                //MY_CLOSE(gi_das_data_fd);
            }
            if( st_conf_tmp.cfu_listen_port!=gst_conf.cfu_listen_port )
            {
                open_cfu_link_port(gst_conf.cfu_listen_port);
            }
        }

        if( strcmp(apc_conf_name[i], "dms.yaml")==0 )
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "ma notice reload dms.yaml config!");
            read_dms_config (DMS_CONFIG_FILE_PATH, &gst_conf);
            MY_CLOSE(gi_ma_fd);
            MY_CLOSE(gi_sguard_fd);
        }
    }

    return 0;    
}

void counter_info_dispose(unsigned short num)
{
    struct counter
    {
        unsigned int ui_das;
        unsigned int ui_svm_fail;
        unsigned int ui_svm_success;
        unsigned int ui_wireless_fail;
        unsigned int ui_wireless_success;
        unsigned int ui_wireless_abnormal;
        unsigned int ui_voip_a_fail;
        unsigned int ui_voip_a_success;
        unsigned int ui_voip_a_abnormal;
        unsigned int ui_voip_b_fail;
        unsigned int ui_voip_b_success;
        unsigned int ui_voip_b_abnormal;
    }tmp,var;
    unsigned char ac_buf[500] = {0};
    unsigned int ui_len = 0;

    tmp.ui_das = get_das_total_rule ();
    tmp.ui_svm_success = get_svm_total_success_rule ();
    tmp.ui_svm_fail = get_svm_total_fail_rule ();
    tmp.ui_wireless_success = get_wireless_total_success_rule ();
    tmp.ui_wireless_fail = get_wireless_total_fail_rule ();
    
    var.ui_das = htonl (tmp.ui_das);
    var.ui_svm_success = htonl (tmp.ui_svm_success);
    var.ui_svm_fail = htonl (tmp.ui_svm_fail);
    var.ui_wireless_fail = htonl (tmp.ui_wireless_fail);
    var.ui_wireless_success = htonl (tmp.ui_wireless_success);
    var.ui_wireless_abnormal = htonl(g_wireless.abnormal_clue_counter);
    var.ui_voip_a_fail = htonl (g_voip_a.counter_send_fail);
    var.ui_voip_a_success = htonl (g_voip_a.counter_send_success);
    var.ui_voip_a_abnormal = htonl(g_voip_a.abnormal_clue_counter);
    var.ui_voip_b_fail = htonl (g_voip_a.counter_send_fail);
    var.ui_voip_b_success = htonl (g_voip_a.counter_send_success);
    var.ui_voip_b_abnormal = htonl(g_voip_a.abnormal_clue_counter);

    if( (ui_len=fill_ma_counter2 (ac_buf, 20, num, (unsigned char *)&var, sizeof(var)))>0 )
    {
        if( sendto_info(gi_ma_fd, ac_buf, ui_len)<=0 )    
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "business counter information packet send to ma failed!");
        }
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "business counter information group packet failed!");
    }
}




#if 0
//ClueCallInfo
struct cmdl_clue
{
    unsigned int CLUE_ID;            //规则号
    unsigned int OBJECT_ID;     //对象号        213874
    unsigned char METHOD;       //匹配方式    0：模糊  1：精确
    unsigned int ID_TYPE;        //布控号码类型
    char CONTENT[256];          //布控号码
};
int cmdl_show_cule(unsigned short us_num, unsigned short us_cmd, container **pcon_array)
{
    unsigned char *puc_pkt = (unsigned char *)MY_MALLOC(sizeof(struct cmdl_clue)*1024);
    unsigned int ui_total_pktlen = 1024;
    struct pkt_header *pst_ph = (struct pkt_header *)(puc_pkt);
    unsigned int ui_pktlen = pkt_head_length;
    int i = 0;
    ClueCallInfo *pst_rule = NULL;
    struct cmdl_rule *pst_simp_rule = NULL;

/*
     uint8_t ver;
    uint8_t reply;
    uint16_t cmd;
    uint16_t num;
    uint16_t len;
 */
    applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "(command line)have a request das rule command!");

    pst_ph->ver = PCKT_HEAD_PRO_VER;
    pst_ph->reply = PH_FLG_REPLY;
    pst_ph->cmd = htons (us_cmd);    
    pst_ph->num = htons (us_num);    
    pst_ph->len = 0;
    
    for( i=0; pcon_array[i]!=NULL; ++i )        
    {
        container_search(pst_rule, pcon_array[i])
        {
            pst_simp_rule = (struct cmdl_rule *)(puc_pkt+ui_pktlen);
            ui_pktlen += sizeof(struct cmdl_rule);

            pst_simp_rule->CLUE_ID = pst_rule->CLUE_ID;    
            pst_simp_rule->OBJECT_ID = pst_rule->OBJECT_ID;    
            pst_simp_rule->METHOD = pst_rule->METHOD;    
            //struct cmdl_clue
            if( pst_rule->ID_TYPE==ID_TYPE_IMSI || pst_rule->ID_TYPE==ID_TYPE_IMEI || \
                pst_rule->ID_TYPE==ID_TYPE_PHONE )
            {
                strcpy (pst_simp_rule->CONTENT, pst_rule->CALL_ID);    
            }
            else if( pst_rule->ID_TYPE==ID_TYPE_KEYWORD )
            {
                strcpy (pst_simp_rule->CONTENT, pst_rule->KEYWORD);    
            }
            else if( pst_rule->ID_TYPE==ID_TYPE_KEYWORD )
            {
            }
        }
    }
    
    sendto_info (gi_ma_fd, ac_buf, pkt_head_length+strlen(ac_path)+1);

    return 0;
}
#endif

int cmdl_show_cule(unsigned short us_num, unsigned short us_cmd, char *pc_filename)
{
    char ac_buf[1024] = {0};
    char ac_path[500] = {0};
    struct pkt_header *pst_ph = (struct pkt_header *)ac_buf;

    pst_ph->ver = PCKT_HEAD_PRO_VER;
    pst_ph->reply = PH_FLG_REPLY;
    pst_ph->cmd = htons (us_cmd);    
    pst_ph->num = htons (us_num);    
    pst_ph->len = 0;
    
    snprintf (ac_path, sizeof(ac_path), "%s%s", gst_conf.ac_conf_path_base, pc_filename);
    strcpy (ac_buf+pkt_head_length, ac_path);        
    pst_ph->len = htons(pkt_head_length+strlen(ac_path)+1);    
    sendto_info (gi_ma_fd, ac_buf, pkt_head_length+strlen(ac_path)+1);

    return 0;
}

int register_counter_info_struct(unsigned short num)
{
    unsigned char ac_pkt[1024] = {0};
    struct count_struct stu[] = {
        {"das_rule", 4},
        {"svm_fail_rule", 4},
        {"svm_success_rule", 4},
        {"wireless_fail_rule", 4},
        {"wireless_success_rule", 4},
        {"wireless_abnormal_rule", 4},
        {"voip_a_fail_rule", 4},
        {"voip_a_success_rule", 4},
        {"voip_a_abnormal_rule", 4},
        {"voip_b_fail_rule", 4},
        {"voip_b_success_rule", 4},
        {"voip_b_abnormal_rule", 4},
    };
    int ret; 

    if( (ret=fill_ma_count_info_struct(ac_pkt, num, "rfu", "rfu", 12, stu))>0 )
    {
        if( sendto_info(gi_ma_fd, ac_pkt, (unsigned int)ret)<=0 )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "send counter information struct register packet failed!");
        }
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "business counter information struct register packet group packet failed!");
    }

    return 0;
}

int ma_data_dispose(fd_set *pst_rfds)
{
    if( gi_ma_fd<=0 || FD_ISSET(gi_ma_fd, pst_rfds)==0 )    
    {
        return -1;
    }

    struct pkt_header st_head;
    unsigned char *pc_data = NULL;    
    unsigned char ac_pkt[200] = {0};
    unsigned int ui_len = 0;
    unsigned int v = 0;

    if( read_ma_data(&st_head, &pc_data) )
    {
        return -1;
    }

    switch (st_head.cmd)
    {
        case CMD_BUS_STATISTICAL: 
            counter_info_dispose(st_head.num);
            break;

        case CMD_ANEW_READ_CONF:
            ma_reload_conf_file (pc_data, (st_head.len-pkt_head_length));        
            break;

        case CMDL_DEBUG_ON: 
            applog_debug_on (&gi_ma_fd);
            break;

        case CMDL_DEBUG_OFF:
            applog_debug_off ();
            break;

        case CMDL_SET_LOG_LEV:
            ui_len = st_head.len-pkt_head_length;
            ma_cmd_get_loglevel (pc_data, ui_len, &v);
            applog_set_log_level (v);
            break;

        case CMDL_SET_LOG_MASK:
            ui_len = st_head.len-pkt_head_length;
            ma_cmd_get_logmask (pc_data, ui_len, &v);
            applog_set_debug_mask (v);
            break;

        case CMDL_SHOW_LOG_LEV:
            ui_len = fill_loglevel_packet(ac_pkt, st_head.num, g_app_log_level);
            sendto_info (gi_ma_fd, ac_pkt, ui_len);
            break;

        case CMDL_SHOW_LOG_MASK:
            ui_len = fill_logmask_packet(ac_pkt, st_head.num, g_app_debug_mask);
            sendto_info (gi_ma_fd, ac_pkt, ui_len);
            break;    
        
        case CMDL_RFU_SHOW_DAS_RULE:
            cmdl_show_cule (st_head.num, CMDL_RFU_SHOW_DAS_RULE, DAS_RULE_SAVE_FILE_NAME);
            break;

        case CMDL_RFU_SHOW_WIRELESS_RULE:
            cmdl_show_cule (st_head.num, CMDL_RFU_SHOW_WIRELESS_RULE, WIRELESS_SVM_SAVE_RULE_FILE_NAME);
            break;
        
        case CMDL_RFU_SHOW_SVM_RULE:
            svm_all_write_xml_file();        
            cmdl_show_cule (st_head.num, CMDL_RFU_SHOW_SVM_RULE, ".svm_rule.xml");
            break;

        case CMD_COUNT_INFO_STATUS_REG:
            register_counter_info_struct (st_head.num);
            break;

        default: 
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "recv a don't know command(cmd: %hu, length: %hu)", st_head.cmd, st_head.len);
    }
    MY_FREE(pc_data);

    return 0;
}


void* pthread_send_alive_to_sguard(void *src)
{
    unsigned char ac_data[500] = {0};
    unsigned int ui_len = 0;

    while( 1 )
    {
        if( gi_sguard_fd>0 )
        {
            memset (ac_data, 0, sizeof(ac_data));
            ui_len = fill_alive_packet (ac_data, 1);
            sendto_info (gi_sguard_fd, ac_data, ui_len);
        }
        sleep(1);
    }
    return NULL;
}


int link_ma()
{
    if( gst_conf.if_link_ma_flg==0 ) 
    {
        return -1;
    }

    unsigned char ac_data[500]= {0};
    unsigned int ui_len = 0;

    if( gi_ma_fd>0 )
    {
        MY_CLOSE(gi_ma_fd);
    }
    if( gi_sguard_fd>0 )
    {
        MY_CLOSE(gi_sguard_fd);
    }

    if( (gi_ma_fd=connect2srv("127.0.0.1", gst_conf.ma_port))<0 )
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "Rfu linked ma failed");
        return -1;    
    }
    if( (ui_len=fill_regma_packet(ac_data, getpid(), gi_sn))>0 )
    {
        sendto_info(gi_ma_fd, ac_data, ui_len);
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call fill_regma_packet function failed!");
    }
        
    if( (gi_sguard_fd=connect2srv("127.0.0.1", gst_conf.sguard_port))<0 )
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "Rfu linked sguard failed");
        MY_CLOSE(gi_ma_fd);
        return -1;    
    }
    memset (ac_data, 0, sizeof(ac_data));
    if( (ui_len=fill_regsguard_packet(ac_data, getpid(), gi_sn, NULL, 0))>0 )
    {
        sendto_info(gi_sguard_fd, ac_data, ui_len);
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call fill_regsguard_packet function failed!");
    }

    return 0;
}

int ma_init()
{
    if( gst_conf.if_link_ma_flg==0 ) 
    {
        return -1;
    }
    
    pthread_t pth;
    link_ma();            
    pthread_create(&pth, NULL, pthread_send_alive_to_sguard, NULL);    
    pthread_detach (pth);

    return 0;
}

int ma_timeout()
{
    if( gst_conf.if_link_ma_flg==0 )
    {
        return 0;
    }

    if( gi_sguard_fd<0 )
    {
        if( link_ma() )
        {
            return -1;
        }
    }

    if( gi_ma_fd<0 )
    {
        if( link_ma() )
        {
            return -1;
        }
    }

    return 0;
}


int ma_add_detect_fd(fd_set *rfds)
{    
    if( rfds==NULL )
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "call %s function failed, argument error!", __FUNCTION__ );
        return -1;
    }
    
    if( gi_ma_fd>0 )
    {
        FD_SET (gi_ma_fd, rfds);
    }

    return 0;
}



