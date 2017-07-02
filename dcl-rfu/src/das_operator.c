
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <ctype.h>
#include <time.h>

#include "variable.h"
#include "das_operator.h"
#include "../../common/sock_cli.h"
#include "svm_operator.h"
#include "wireless_svm_operator.h"
#include "container.h"
#include "ods_body.h"


/***********************************************************************************************/
int save_rule_to_container(container *save_container, ClueCallInfo *src)
{
    if( save_container==NULL || src==NULL )
    {
        return -1;
    }
    container_add_data (save_container, ods_memcpy(src));
    return 0;
}

int save_rule_to_container_wireless(container *save_container, ClueCallInfo *src)
{
    if( save_container==NULL || src==NULL )
    {
        return -1;
    }
    ClueCallInfo *tmp = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));        
    *tmp = *src;
    container_add_data (save_container, tmp);
    return 0;
}

struct rule_operator_struct
{
    int restore_sys;    
    container *save_container;
    int (*pf_judge)(ClueCallInfo *src);
    int (*pf_save)(container *save_container, ClueCallInfo *src);
    int (*pf_dispose_fun)(container *save_container);
};

struct rule_operator_struct g_rule_ope_array[] = {
        {RESTORE_SYS_SVM, 0, svm_spu_judge, save_rule_to_container, svm_spu_dispose},
        {RESTORE_SYS_SVM, 0, svm_mpu_phone_judge, save_rule_to_container, svm_mpu_phone_dispose},
        {RESTORE_SYS_SVM, 0, svm_mpu_content_judge, save_rule_to_container, svm_mpu_content_dispose},
        {RESTORE_SYS_SVM, 0, svm_vrs_judge, save_rule_to_container, svm_vrs_dispose},
        {RESTORE_SYS_WIRELESS, 0, wireless_ismi_judge, save_rule_to_container_wireless, wireless_imsi_dispose},
        {RESTORE_SYS_VOIP_A, 0, wireless_ismi_judge, save_rule_to_container_wireless, voip_a_dispose},
        {RESTORE_SYS_VOIP_B, 0, wireless_ismi_judge, save_rule_to_container_wireless, voip_b_dispose}
    };
#define rule_ope_array_size (sizeof(g_rule_ope_array)/sizeof(g_rule_ope_array[0]))

int operator_struct_init()
{
    int i;    
    for( i=0; i<rule_ope_array_size; ++i )
    {
        g_rule_ope_array[i].save_container = create_container();     
    }
    return 0;
}
/****************************************************************************************/

container *das_rule_save = NULL;
int gi_das_listen_fd = -1;
int gi_cfu_listen_fd = -1; 
int gai_cfu_link_fd[100] = {0};
int gai_das_link_fd[100] = {0};
static int gi_link_signal_flg = 0;
unsigned int parse_rule_error_num = 0;

int clean_fd_content(int fd)
{
    if( fd<0 )
    {
        return -1;
    }

    char ac_buf[1024] = {0};    
    fd_set rfd;
    struct timeval tv = {0,0};
    int ret = 0;
    
    do{
        FD_ZERO (&rfd);
        FD_SET (fd, &rfd);    
        ret = select (1024, &rfd, NULL, NULL, &tv);
        if( ret>0 )
        {
            recv(fd, ac_buf, sizeof(ac_buf), 0);
        }
    }while( ret>0 );

    return 0;
}


int das_read_data(int *pi_fd, int *gi_opt_mode, char **pp_xml_buf, int *pi_xml_len)
{
    /*
    思路:
        1. 先读socket缓冲区的5个字节，获得xml的长度和操作类型
        2. 为xml分配内存，*pp_xml_buf存储首地址, *pi_xml_len存储分配内存大小
        3. 循环读socket，直到剩余长度为0
        3. surplus_rule_len减去读到到长度(最好还是判断相减会不会是负数)
        4. 如果 surplus_rule_len 等于0， 表示包接收完成
        5. 若果surplus_rule_len 不等于0， 返回-1.
    返回值:
        -1 recv出错
        -2 内存分配出错
        -3 下发的规则有问题
        -4 参数错误
        0 规则接收成功
        1 规则没有完全接受
        2 tcp链接断开
    */
    char ac_data[5] = {0};  //读socket的前5个字节，包含了1个字节的类型，4个字节的xml长度
    int recv_len = 0;
    int surplus_rule_len = 0;

    if(pp_xml_buf == NULL || pi_xml_len == NULL)
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "invalid parameter."); 
        return -4;
    }
    
    if( (recv_len=recv(*pi_fd, ac_data, sizeof(ac_data), 0))<0 )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call recv(das data, packet body) functioan failed, %s(%d)!", strerror(errno), *pi_fd); 
        MY_CLOSE ((*pi_fd));
        return -1;
    }
    else if( recv_len != 5 )
    {
        MY_CLOSE ((*pi_fd));    //break tcp link
        return 2;
    }

    //读xml的长度
    *gi_opt_mode = ac_data[0] - '0';
    surplus_rule_len = ntohl(*((int *)(&ac_data[1])));
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "recv %s rule, xml len : %d.", *gi_opt_mode == 1 ? "full" : "increment", surplus_rule_len);

    //为xml分配内存，多分配一个字节，最后为\0
    *pp_xml_buf = (char *)malloc(surplus_rule_len + 1);
    if(*pp_xml_buf == NULL)
    {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "malloc memory failed!!!!");
        MY_CLOSE((*pi_fd));
        return -2;
    }
    bzero(*pp_xml_buf, surplus_rule_len + 1);
    *pi_xml_len = surplus_rule_len + 1;
    applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "malloc memory for xml done, %d bytes.", surplus_rule_len + 1);

    //读socket缓冲的xml
    recv_len = recv(*pi_fd, *pp_xml_buf, surplus_rule_len, 0);
    if(recv_len < 0)
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call recv(das data, packet body) functioan failed, %s(%d)!", strerror(errno), *pi_fd); 
        free(*pp_xml_buf);
        *pp_xml_buf = NULL;
        MY_CLOSE ((*pi_fd));
        return -1;
    }
    
    if(recv_len < surplus_rule_len)
    {
        MY_CLOSE(*pi_fd);
        free(*pp_xml_buf);
        *pp_xml_buf = NULL;
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "bad rule xml!!!!");
        return 1;
    }

    return 0;

}

int das_rule_save_to_stuct(xmlNodePtr pnode_attr, ClueCallInfo *pst_rule_info_ret)
{
    xmlAttrPtr pattr_var = pnode_attr->properties;
    xmlChar *pc_content;
    int flg = 0;

    if( pattr_var==NULL || pst_rule_info_ret==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function filaed, argument error!", __FUNCTION__); 
        return -1;
    }

    while( pattr_var!=NULL )
    {
        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"CLUE_TYPE")==0 )
        {
            flg = flg | (1<<0);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"CLUE_TYPE");
            strncpy (pst_rule_info_ret->CLUE_TYPE, (const char*)pc_content, sizeof(pst_rule_info_ret->CLUE_TYPE)-1);
            xmlFree(pc_content);
        }

        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"CLUE_STATUS")==0 )
        {
            flg = flg | (1<<1);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"CLUE_STATUS");
            pst_rule_info_ret->CLUE_STATUS = atoi((const char*)pc_content);
            xmlFree(pc_content);
        }

        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"CLUE_ID")==0 )
        {
            flg = flg | (1<<2);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"CLUE_ID");
            pst_rule_info_ret->CLUE_ID = atoi((const char *)pc_content);
            xmlFree(pc_content);
        }

        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"CALL_ID")==0 )
        {
            flg = flg | (1<<3);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"CALL_ID");
            if( strlen((const char *)pc_content)>0 )
            {
                strncpy (pst_rule_info_ret->CALL_ID, (const char *)pc_content, sizeof(pst_rule_info_ret->CALL_ID)-1);
            }
            else
            {
                strncpy (pst_rule_info_ret->CALL_ID, "100", sizeof(pst_rule_info_ret->CALL_ID)-1);
            }
            xmlFree(pc_content);
        }

        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"KEYWORD")==0 )
        {
            flg = flg | (1<<4);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"KEYWORD");
            strncpy (pst_rule_info_ret->KEYWORD, (const char *)pc_content, sizeof(pst_rule_info_ret->KEYWORD)-1);
            xmlFree(pc_content);
        }

        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"CB_ID")==0 )
        {
            flg = flg | (1<<5);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"CB_ID");
            pst_rule_info_ret->CB_ID = atoi((const char *)pc_content);
            xmlFree(pc_content);
        }

        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"ID_TYPE")==0 )
        {
            flg = flg | (1<<6);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"ID_TYPE");
            pst_rule_info_ret->ID_TYPE = atoi((const char *)pc_content);
            xmlFree(pc_content);
        }
        
        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"ACTION_TYPE")==0 )
        {
            flg = flg | (1<<7);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"ACTION_TYPE");
            pst_rule_info_ret->ACTION_TYPE = atoi((const char *)pc_content);
            xmlFree(pc_content);
        }

        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"LIFT_TIME")==0 )
        {
            flg = flg | (1<<8);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"LIFT_TIME");
            strncpy (pst_rule_info_ret->LIFT_TIME, (const char *)pc_content, sizeof(pst_rule_info_ret->LIFT_TIME)-1);
            xmlFree(pc_content);
        }

        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"METHOD")==0 )
        {
            flg = flg | (1<<9);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"METHOD");
            pst_rule_info_ret->METHOD = atoi((const char *)pc_content);
            xmlFree(pc_content);
        }

        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"OBJECT_ID")==0 )
        {
            flg = flg | (1<<10);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"OBJECT_ID");
            pst_rule_info_ret->OBJECT_ID = atoi((const char *)pc_content);
            xmlFree(pc_content);
        }
        
        if( xmlStrcmp(pattr_var->name, (const xmlChar *)"PROTOCOL_LIST")==0 )
        {
            flg = flg | (1<<11);
            pc_content = xmlGetProp(pnode_attr, BAD_CAST"PROTOCOL_LIST");
            strncpy (pst_rule_info_ret->PROTOCOL_LIST, (const char *)pc_content, sizeof(pst_rule_info_ret->PROTOCOL_LIST)-1);
            xmlFree(pc_content);
        }

        pattr_var = pattr_var->next;
    }
    
    if( flg!=0xfff )
    {
        if( (flg & (1<<0))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not CLUE_TYPE option!");
        }
        if( (flg & (1<<1))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not CLUE_STATUS option!");
        }
        if( (flg & (1<<2))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not CLUE_ID option!");
        }
        if( (flg & (1<<3))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not CALL_ID option!");
        }
        if( (flg & (1<<4))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not KEYWORD option!");
        }
        if( (flg & (1<<5))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not CB_ID option!");
        }
        if( (flg & (1<<6))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not ID_TYPE option!");
        }
        if( (flg & (1<<7))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not ACTION_TYPE option!");
        }
        if( (flg & (1<<8))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not LIFT_TIME option!");
        }
        if( (flg & (1<<9))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not METHOD option!");
        }
        if( (flg & (1<<10))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not OBJECT_ID option!");
        }
        if( (flg & (1<<11))==0 )
        {
            applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "have a rule not PROTOCOL_LIST option!");
        }
        return -1;
    }
    else
    {
        //applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "[debug] rule format ClueCallInfo success (das, CLUE_ID %u, ID_TYPE: %u, CALL_ID: %s, CLUE_STATUS: %d)!", pst_rule_info_ret->CLUE_ID, pst_rule_info_ret->ID_TYPE, pst_rule_info_ret->CALL_ID, pst_rule_info_ret->CLUE_STATUS);
        return 0;
    }
}


int das_rule_to_container(container *pst_rule_info_ret, char *xmlbuf, int xmlbuf_len)
{
    if( pst_rule_info_ret==NULL || xmlbuf == NULL)
    {
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "call %s functioan failed, argument error!", __FUNCTION__); 
        return -1;
    }
            
    xmlDocPtr pdoc;    
    xmlNodePtr pnode_root;
    ClueCallInfo *pst_rule_info_tmp = NULL;
    
    if( (pdoc=xmlReadMemory (xmlbuf, xmlbuf_len-1, NULL, "UTF-8", XML_PARSE_RECOVER))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call xmlReadMemory() function failed");
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "xml buffer len: %d", xmlbuf_len);
        FILE * xml_dump_fp = fopen("/tmp/xml_dup.xml", "w+");
        fwrite(xmlbuf, 1, xmlbuf_len, xml_dump_fp);
        fclose(xml_dump_fp);
        return -1;    
    }
    
    if( (pnode_root=xmlDocGetRootElement(pdoc))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call xmlDocGetRootElement() functioan failed"); 
        xmlFreeDoc (pdoc);
        return -1;
    }
    
    if( xmlStrcmp(pnode_root->name, BAD_CAST"CLUE_INFO")!=0 )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "root node name not is \"CLUE_INFO\""); 
        xmlFreeDoc(pdoc);
        return -1;
    }
    
    pnode_root = pnode_root->xmlChildrenNode;
    while( pnode_root!=NULL )
    {
        if( xmlHasProp(pnode_root, BAD_CAST"CLUE_TYPE") )
        {
            pst_rule_info_tmp = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));
            if( (das_rule_save_to_stuct(pnode_root, pst_rule_info_tmp))==0 )
            {
                applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "parse xml CLUE_INFO record: id=%d, status=%d, call_id=%s, keyword=%s", pst_rule_info_tmp->CLUE_ID, pst_rule_info_tmp->CLUE_STATUS, pst_rule_info_tmp->CALL_ID, pst_rule_info_tmp->KEYWORD);
                container_add_data (pst_rule_info_ret, pst_rule_info_tmp);
            }
            else
            {
                xmlBufferPtr nodeBuffer = xmlBufferCreate();
                xmlNodeDump(nodeBuffer, pdoc, pnode_root, 0, 0);
                applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "have a rule string format ClueCallInfo failed, node string: %s", (char *)nodeBuffer->content); 
                xmlBufferFree(nodeBuffer);  
                ods_free(pst_rule_info_tmp);
            }
        }
        pnode_root = pnode_root->next;
    }
    xmlFreeDoc (pdoc);

    return 0;
}

int debug_show_rule(ClueCallInfo *pst_src, int flg)
{
    if( flg==0 )
    {
        printf ("CLUE_ID: %u\n", pst_src->CLUE_ID);    
        printf ("CLUE_STATUS: %d\n", pst_src->CLUE_STATUS);    
        printf ("CLUE_TYPE: %s\n", pst_src->CLUE_TYPE);    
        printf ("CALL_ID: %s\n", pst_src->CALL_ID);    
        printf ("KEYWORD: %s\n", pst_src->KEYWORD);    
        printf ("CB_ID: %d\n", pst_src->CB_ID);    
        printf ("ID_TYPE: %d\n", pst_src->ID_TYPE);    
        printf ("ACTION_TYPE: %d\n", pst_src->ACTION_TYPE);    
        printf ("LIFT_TIME: %s\n", pst_src->LIFT_TIME);    
        printf ("METHOD: %d\n", pst_src->METHOD);    
        printf ("OBJECT_ID: %d\n", pst_src->OBJECT_ID);    
        printf ("PROTOCOL_LIST: %s\n", pst_src->PROTOCOL_LIST);    

        fflush (stdout);
    }
    else
    {
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "CLUE_ID: %u\n", pst_src->CLUE_ID);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "CLUE_STATUS: %d\n", pst_src->CLUE_STATUS);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "CLUE_TYPE: %s\n", pst_src->CLUE_TYPE);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "CALL_ID: %s\n", pst_src->CALL_ID);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "KEYWORD: %s\n", pst_src->KEYWORD);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "CB_ID: %d\n", pst_src->CB_ID);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "ID_TYPE: %d\n", pst_src->ID_TYPE);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "ACTION_TYPE: %d\n", pst_src->ACTION_TYPE);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "LIFT_TIME: %s\n", pst_src->LIFT_TIME);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "METHOD: %d\n", pst_src->METHOD);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "OBJECT_ID: %d\n", pst_src->OBJECT_ID);    
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "PROTOCOL_LIST: %s\n", pst_src->PROTOCOL_LIST);    
    }

    return 0;
}

int debug_show_container_rule(container *pcon_src, int log_flg)
{
    if( pcon_src==NULL )
    {
        printf ("argument is null!\n");
        return -1;
    }

    ClueCallInfo *pst_rule = NULL;
    int sum = 0;
    
    if( container_is_null(pcon_src) )
    {
        printf ("rule total: 0\n");
        return 0;
    }
    
    container_search(pst_rule, pcon_src)
    {
        sum++;
        debug_show_rule(pst_rule, log_flg);
        printf ("=====================%d=========================\n", sum);
    }

    return sum;
}


#define node_add_attribute(pnode_tmp, pst_rule) \
do{\
    char ac_tmp[500] = {0};\
    xmlNewProp (pnode_tmp, BAD_CAST"CLUE_TYPE", (xmlChar *)(pst_rule->CLUE_TYPE));\
    snprintf (ac_tmp, sizeof(ac_tmp)-1, "%u", (unsigned int)(pst_rule->CLUE_STATUS));\
    xmlNewProp (pnode_tmp, BAD_CAST"CLUE_STATUS", (xmlChar *)ac_tmp);\
    snprintf (ac_tmp, sizeof(ac_tmp)-1, "%u", pst_rule->CLUE_ID);\
    xmlNewProp (pnode_tmp, BAD_CAST"CLUE_ID", (xmlChar *)(ac_tmp));\
    xmlNewProp (pnode_tmp, BAD_CAST"CALL_ID", (xmlChar *)(pst_rule->CALL_ID));\
    xmlNewProp (pnode_tmp, BAD_CAST"KEYWORD", (xmlChar *)(pst_rule->KEYWORD));\
    snprintf (ac_tmp, sizeof(ac_tmp)-1, "%u", pst_rule->CB_ID);\
    xmlNewProp (pnode_tmp, BAD_CAST"CB_ID", (xmlChar *)(ac_tmp));\
    snprintf (ac_tmp, sizeof(ac_tmp)-1, "%u", pst_rule->ID_TYPE);\
    xmlNewProp (pnode_tmp, BAD_CAST"ID_TYPE", (xmlChar *)(ac_tmp));\
    snprintf (ac_tmp, sizeof(ac_tmp)-1, "%u", pst_rule->ACTION_TYPE);\
    xmlNewProp (pnode_tmp, BAD_CAST"ACTION_TYPE", (xmlChar *)(ac_tmp));\
    xmlNewProp (pnode_tmp, BAD_CAST"LIFT_TIME", (xmlChar *)(pst_rule->LIFT_TIME));\
    snprintf (ac_tmp, sizeof(ac_tmp)-1, "%d", (int)pst_rule->METHOD);\
    xmlNewProp (pnode_tmp, BAD_CAST"METHOD", (xmlChar *)(ac_tmp));\
    snprintf (ac_tmp, sizeof(ac_tmp)-1, "%u", pst_rule->OBJECT_ID);\
    xmlNewProp (pnode_tmp, BAD_CAST"OBJECT_ID", (xmlChar *)(ac_tmp));\
    xmlNewProp (pnode_tmp, BAD_CAST"PROTOCOL_LIST", (xmlChar *)(pst_rule->PROTOCOL_LIST));\
}while(0)

int create_rule_file_array(char *pc_file_path, container **pcon_rule_array)
{
    if( pc_file_path==NULL || pcon_rule_array==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed, argument error!", __FUNCTION__);
        return -1;
    }
    
    xmlDocPtr pdoc;
    xmlNodePtr pnode_root;
    xmlNodePtr pnode_tmp;
    ClueCallInfo *pst_rule;
    int xml_file_size = -1;
    int counter = 0;
    int i;

    if( (pdoc=xmlNewDoc(BAD_CAST"1.0"))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call xmlNewDoc(%s) function failed!", pc_file_path);
        return -1;
    }
    pnode_root = xmlNewNode (NULL, BAD_CAST"CLUE_INFO");
    xmlDocSetRootElement (pdoc, pnode_root);    
    
    for( i=0; pcon_rule_array[i]!=NULL; i++ )    
    {
        container_search(pst_rule, pcon_rule_array[i])
        {
            pnode_tmp = xmlNewNode (NULL, BAD_CAST"RULE");
            node_add_attribute(pnode_tmp, pst_rule);
            xmlAddChild (pnode_root, pnode_tmp);
            counter++;
        }
    }

    if( (xml_file_size=xmlSaveFile(pc_file_path, pdoc))==-1 )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "Clue write %s xml file failed!", pc_file_path);
    }
    else
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "Clue write %s xml file success, clue numbler: %d, file size: %d!", \
                pc_file_path, counter, xml_file_size);
    }
    xmlFreeDoc (pdoc);

    return 0;
}



int create_rule_file(char *pc_file_path, container *pcon_rule)
{
    if( pc_file_path==NULL || pcon_rule==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed, argument error!", __FUNCTION__);
        return -1;
    }
    
    xmlDocPtr pdoc;
    xmlNodePtr pnode_root;
    xmlNodePtr pnode_tmp;
    ClueCallInfo *pst_rule;
    int xml_file_size = -1;
    int counter = 0;

    if( (pdoc=xmlNewDoc(BAD_CAST"1.0"))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call xmlNewDoc(%s) function failed!", pc_file_path);
        return -1;
    }
    pnode_root = xmlNewNode (NULL, BAD_CAST"CLUE_INFO");
    xmlDocSetRootElement (pdoc, pnode_root);    
    
    container_search(pst_rule, pcon_rule)
    {
        pnode_tmp = xmlNewNode (NULL, BAD_CAST"RULE");
        node_add_attribute(pnode_tmp, pst_rule);
        xmlAddChild (pnode_root, pnode_tmp);
        counter++;
    }
    
    if( (xml_file_size=xmlSaveFile(pc_file_path, pdoc))==-1 )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "Clue write %s xml file failed, clue numbler: %d!", pc_file_path, counter);
    }
    else
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "Clue write %s xml file success, clue numbler: %d, file size: %d!", \
                pc_file_path, counter, xml_file_size);
    }
    xmlFreeDoc (pdoc);

    return counter;
}


int global_dedup_dispose(container *con_das_rule, container *con_local_rule)
{
    if( con_das_rule==NULL || con_local_rule==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed, argument error!", __FUNCTION__);
        return -1;
    }
    
    ClueCallInfo *pst_das_rule = NULL;
    ClueCallInfo *pst_local_rule = NULL;
    container *pcon_tmp = create_container ();
    
    //本地有这个条规则但是在das中没有找到，说明这条规则已经被撤销了.对后面的系统也要进行撤销操作
#if 1
    ClueCallInfo *pst_rule_tmp = NULL;
    container_search(pst_local_rule, con_local_rule)
    {
        int flg = 0;
        container_search(pst_das_rule, con_das_rule)
        {
            if( pst_das_rule->CLUE_ID==pst_local_rule->CLUE_ID )
            {
                flg = 1;
            }
        }
        if( flg==0 )
        {
            pst_rule_tmp = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));
            *pst_rule_tmp = *pst_local_rule;
            pst_rule_tmp->CLUE_STATUS = CLUE_STATUS_DEL;
            container_add_data (pcon_tmp, pst_rule_tmp);
        }
    }
#endif
    
    //清除掉那些在本地有在das全量里面也有的规则
    container_search(pst_das_rule, con_das_rule)
    {
        int flg;
        flg = 0;
        container_search(pst_local_rule, con_local_rule)
        {
            if( pst_das_rule->CLUE_ID==pst_local_rule->CLUE_ID && pst_das_rule->CLUE_STATUS!=CLUE_STATUS_DEL )
            {
                ods_free (pst_das_rule);
                container_free_node(con_das_rule);
                break;
            }
            if(  pst_das_rule->CLUE_ID==pst_local_rule->CLUE_ID && pst_das_rule->CLUE_STATUS==CLUE_STATUS_DEL )
            {
                flg = 1;
            }
        }
        if( pst_das_rule!=NULL &&  flg==0 && pst_das_rule->CLUE_STATUS==CLUE_STATUS_DEL )
        {//清除那些在全量里面测控，但是本地却没有的规则
            //applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "In das gobal clue have delete clue but local not the clue(das, CLUE_ID %u, ID_TYPE: %u, CALL_ID: %s, CLUE_STATUS: %d)", pst_das_rule->CLUE_ID, pst_das_rule->ID_TYPE, pst_das_rule->CALL_ID, pst_das_rule->CLUE_STATUS);
            ods_free (pst_das_rule);
            container_free_node(con_das_rule);
        }
    }
    if( !container_is_null(pcon_tmp) )
    {
        container_catenate (con_das_rule, pcon_tmp);    
    }
    container_destory (pcon_tmp);
    
    return 0;
}

void send_data_to_cfu_single(ClueCallInfo *pst_src)
{
    char ac_buf[100] = {0};
    
    if( pst_src->OBJECT_ID==WHITE_LIST_OBJECT_ID )
    {
        return ;
    }

    *(unsigned short*)ac_buf = 0xffff;
    *(unsigned short *)(ac_buf+2) = htons(sizeof(unsigned int)*3);
    if( pst_src->CLUE_STATUS==CLUE_STATUS_ADD )
    {
        *(unsigned int *)(ac_buf+4) = htonl(1);
    }
    else if( pst_src->CLUE_STATUS==CLUE_STATUS_DEL )
    {
        *(unsigned int *)(ac_buf+4) = htonl(0);
    }
    else
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "In the %s function, ClueCallInfo element value error(CLUE_STATUS=%d)!", \
                __FUNCTION__, pst_src->CLUE_STATUS);
        return ;
    }
    *(unsigned int *)(ac_buf+8) = htonl(pst_src->OBJECT_ID);
    *(unsigned int *)(ac_buf+12) = htonl(pst_src->CLUE_ID); 
    
    int i;            
    for( i=0; i<sizeof(gai_cfu_link_fd)/sizeof(gai_cfu_link_fd[0]); ++i )
    {
        if( gai_cfu_link_fd[i]>0 )
        {
            if( send(gai_cfu_link_fd[i], ac_buf, 16, 0)<=0 )
            {
                applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call send(cfu information) function fail, %s!", strerror(errno));
                MY_CLOSE(gai_cfu_link_fd[i]);    
            }
            else
            {
                applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "sendto cfu id(clude id: %u, object id: %u)", pst_src->CLUE_ID, pst_src->OBJECT_ID);
            }
        }
    }
}


int das_rule_dispose(int *pi_fd)
{
    int rule_opt_type = -1;
    container *pcon_rule_info = create_container (); //alloc memory
    ClueCallInfo *pst_rule_tmp = NULL;
    char ac_file_path[500] = {0};
    int i = 0;
    int ret = -1;
    char *xmlbuf = NULL;    //读socket获得的xml数据存储位置
    int xmlbuf_len = 0;         //xmlbuf_len的长度
    
    if( (ret=das_read_data(pi_fd, &rule_opt_type, &xmlbuf, &xmlbuf_len))!=0 )
    {
        if( ret==-3 )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "have a das issue rule format error!");
            parse_rule_error_num++;
        }
        else if( ret!=1 && ret!=2 )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "have a rule parse proccess error!");
        }
        return -1;
    }
    else
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "das rule save file success(%s%s)!", \
                gst_conf.ac_conf_path_base, DAS_TMP_RULE_FILE_NAME);
    }
    
    if( das_rule_to_container(pcon_rule_info, xmlbuf, xmlbuf_len)!=0 )
    {
        parse_rule_error_num++;
        free(xmlbuf);
        xmlbuf = NULL;
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "das_rule_string_to_stuct function get rule failed!");
        return -1;
    }
    else
    {
        free(xmlbuf);
        xmlbuf = NULL;
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "[debug]das rule file format container success!");
    }

    
    if( rule_opt_type==1 )
    {//全量
        global_dedup_dispose(pcon_rule_info, das_rule_save);
        applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "[debug]global dispose finish!");
    }
        
    applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "[debug]rule alloc restore system!");
    container_search(pst_rule_tmp, pcon_rule_info)
    {
        //son system save rule
        for( i=0; i<rule_ope_array_size; ++i )            
        {
            if( (*g_rule_ope_array[i].pf_judge)(pst_rule_tmp) )
            {
                (*g_rule_ope_array[i].pf_save)(g_rule_ope_array[i].save_container, pst_rule_tmp);
            }
        }

        send_data_to_cfu_single(pst_rule_tmp);
        if( pst_rule_tmp->CLUE_STATUS==CLUE_STATUS_ADD )
        {
            container_free_node (pcon_rule_info);
            container_add_data (das_rule_save, pst_rule_tmp);
        }
        else if( pst_rule_tmp->CLUE_STATUS==CLUE_STATUS_DEL )
        {
            ClueCallInfo *pst_old_rule = NULL;
            int flg = 0;
            container_search(pst_old_rule, das_rule_save)
            {
                if( pst_old_rule->CLUE_ID==pst_rule_tmp->CLUE_ID  )
                {
                    container_free_node (das_rule_save);
                    ods_free(pst_old_rule);
                    flg = 1;
                }
            }
            if( flg==0 )
            {
                applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "a revocation of the rules, but the rules do not take effect(das, CLUE_ID %u, ID_TYPE: %u, CALL_ID: %s, CLUE_STATUS: %d)", \
                        pst_rule_tmp->CLUE_ID, pst_rule_tmp->ID_TYPE, pst_rule_tmp->CALL_ID, pst_rule_tmp->CLUE_STATUS);
            }
            ods_free(pst_rule_tmp);
            container_free_node (pcon_rule_info);
        }
        else
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "a revocation of the rules, but the rules do not take effect(CLUE_ID %u, ID_TYPE: %u, CALL_ID: %s, CLUE_STATUS: %d)", \
                    pst_rule_tmp->CLUE_ID, pst_rule_tmp->ID_TYPE, pst_rule_tmp->CALL_ID, pst_rule_tmp->CLUE_STATUS);
            ods_free(pst_rule_tmp);
            container_free_node (pcon_rule_info);
        }

    }
    
    //update rule file
    snprintf (ac_file_path, sizeof(ac_file_path)-1, "%s%s", gst_conf.ac_conf_path_base, DAS_RULE_SAVE_FILE_NAME);
    create_rule_file (ac_file_path, das_rule_save);
    
    //rule operator 
    for( i=0; i<rule_ope_array_size; ++i )            
    {
        if( !container_is_null(g_rule_ope_array[i].save_container) )
        {
            (*g_rule_ope_array[i].pf_dispose_fun)(g_rule_ope_array[i].save_container);
        }
    }
    
    container_destory (pcon_rule_info);

    return 0;
}

int send_data_to_cfu_container(container *pcon_src, int fd)
{
    ClueCallInfo *pst_tmp = NULL;    
    char ac_buf[1024] = {0};
    char *pc_tmp = ac_buf+4;
    unsigned short ui_length = 0;    
    
    container_search(pst_tmp, pcon_src) 
    {
        if( pst_tmp->OBJECT_ID==WHITE_LIST_OBJECT_ID )
        {
            continue;
        }

        if( pst_tmp->CLUE_STATUS==CLUE_STATUS_ADD )
        {
            *(unsigned int *)(pc_tmp) = htonl(1);            
        }
        else if( pst_tmp->CLUE_STATUS==CLUE_STATUS_DEL )
        {
            *(unsigned int *)(pc_tmp) = htonl(0);            
        }
        else
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "In the %s function, ClueCallInfo element value error(CLUE_STATUS=%d)!", \
                __FUNCTION__, pst_tmp->CLUE_STATUS);
            continue;
        }
        *(unsigned int *)(pc_tmp+4) = htonl(pst_tmp->OBJECT_ID);            
        *(unsigned int *)(pc_tmp+8) = htonl(pst_tmp->CLUE_ID);
        pc_tmp += (sizeof(unsigned int)*3);
        ui_length += (sizeof(unsigned int)*3);

        if( ui_length>=1020 ) 
        {
            *(unsigned short *)(ac_buf) = 0xffff;
            *(unsigned short *)(ac_buf+2) = htons(ui_length);
            if( send(fd, ac_buf, ui_length+4, 0)<=0 )
            {
                applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call send(cfu information) function fail, %s!", strerror(errno));
                MY_CLOSE(fd);    
                return -1;
            }
            
            memset(ac_buf, 0, sizeof(ac_buf));
            pc_tmp = ac_buf+4;
            ui_length = 0;
        }
    }
    
    if( ui_length!=0 )
    {
        *(unsigned short *)(ac_buf) = 0xffff;
        *(unsigned short *)(ac_buf+2) = htons(ui_length);
        if( send(fd, ac_buf, ui_length+4, 0)<=0 )
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call send(cfu information) function fail, %s!", strerror(errno));
            MY_CLOSE(fd);    
            return -1;
        }
    }

    return 0;
}

int das_info_dispose(fd_set *pst_rfds)
{
    int fd;
    int i;

    if( gi_das_listen_fd>0 && (FD_ISSET(gi_das_listen_fd, pst_rfds))!=0 )
    {
        if( (fd=accept(gi_das_listen_fd, NULL, NULL))>0 )    
        {
            applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "have a new das link!");
            for( i=0; i<sizeof(gai_das_link_fd)/sizeof(gai_das_link_fd[0]); ++i )    
            {
                if( gai_das_link_fd[i]<=0 )
                {
                    gai_das_link_fd[i] = fd;    
                    break;
                }
            }
            if( i==sizeof(gai_das_link_fd)/sizeof(gai_das_link_fd[0]) )    
            {
                applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "das number of connected to more than 100!");
                close (fd);
            }
        }
        else
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call accept(das link: %d) function fail, %s!", fd, strerror(errno));
        }
    }
    
    for( i=0; i<sizeof(gai_das_link_fd)/sizeof(gai_das_link_fd[0]); ++i )
    {
        if( gai_das_link_fd[i]>0 && (FD_ISSET(gai_das_link_fd[i], pst_rfds))!=0 )
        {
            das_rule_dispose(&gai_das_link_fd[i]);
        }
    }

    if( gi_cfu_listen_fd>0 && (FD_ISSET(gi_cfu_listen_fd, pst_rfds))!=0 )
    {
        int fd = -1;
        char ac_buf[4] = {0};
        
        *(unsigned short*)ac_buf = 0xffff;
        *(unsigned short*)(ac_buf+2) = 0;

        if( (fd=accept(gi_cfu_listen_fd, NULL, NULL))>0 )
        {
            applog (APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_RFU, "have a cfu link!");
            for( i=0; i<sizeof(gai_cfu_link_fd)/sizeof(gai_cfu_link_fd[0]); ++i )    
            {
                if( gai_cfu_link_fd[i]>0 )
                {
                    //judge link if break
                    if( send(gai_cfu_link_fd[i], ac_buf, 4, MSG_NOSIGNAL)<=0 )
                    {
                        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "send(judge cuf link break) to data fail, %s!", strerror(errno));
                        MY_CLOSE(gai_cfu_link_fd[i]);
                    }
                }
                if( gai_cfu_link_fd[i]<=0 )    
                {
                    if( send_data_to_cfu_container(das_rule_save, fd)==0 )
                    {
                        gai_cfu_link_fd[i] = fd;
                    }
                    break;
                }
            }
            if( i==sizeof(gai_cfu_link_fd)/sizeof(gai_cfu_link_fd[0]) )
            {
                applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "CFU number of connected to more than 100!");
                MY_CLOSE(fd);
            }
        }
        else
        {
            applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call accept(cfu link: %d) function fail, %s!", fd, strerror(errno));
        }
    }

    return 0;
}

int create_tcp_server(char *ip, uint16_t port)
{
    int listenfd;
    struct sockaddr_in servaddr;
    int reuseaddr = 1;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "create_tcp_server create socket fail");
        return -1;
    }

    if (port == 0) {
        port = gst_conf.das_data_port;
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    if (ip == NULL) {
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        servaddr.sin_addr.s_addr = inet_addr(ip);
    }
    servaddr.sin_port = htons(port);

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "create_tcp_server bind socket fail");
        close(listenfd);
        return -1;
    }

    if(listen(listenfd, 2048) == -1){
        applog(APP_LOG_LEVEL_DEBUG, APP_VPU_LOG_MASK_BASE, "create_tcp_server listen socket fail");
        close(listenfd);
        return -1;
    }
    
    return listenfd;    
}

int open_cfu_link_port(int port)
{
    int i;
    if( gi_cfu_listen_fd>0 )
    {
        MY_CLOSE(gi_cfu_listen_fd);
    }
    for( i=0; i<sizeof(gai_cfu_link_fd)/sizeof(gai_cfu_link_fd[0]); i++ )
    {
        if( gai_cfu_link_fd[i]>0 )
        {
            MY_CLOSE(gai_cfu_link_fd[i]);
        }
    }
    
    if( (gi_cfu_listen_fd=create_tcp_server(NULL, port))<0 )
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "create socket(cfu listen,port: %d) server failed!", port);
        exit (-1);    
    }
    
    return 0;
}

int open_das_link_port(int port)
{
    int i;

    if( gi_das_listen_fd>0 )
    {
        MY_CLOSE(gi_das_listen_fd);
    }

    for( i=0; i<sizeof(gai_das_link_fd)/sizeof(gai_das_link_fd[0]); i++ )
    {
        if( gai_das_link_fd[i]>0 )
        {
            MY_CLOSE(gai_das_link_fd[i]);
        }
    }
    
    if( (gi_das_listen_fd=create_tcp_server(NULL, port))<0 )
    {
        applog (APP_LOG_LEVEL_ERR, APP_LOG_MASK_RFU, "create socket(das listen,port: %d) server failed!", port);
        exit (-1);    
    }
    
    return 0;
}


int read_rule_xml_file(char *pc_file_path, void (*pf_opt_fun)(ClueCallInfo *))
{
    if( pc_file_path==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call %s function failed, argument error!", __FUNCTION__);
        return -1;
    }
    
    xmlDocPtr pdoc;        
    xmlNodePtr pnode_root;    
    ClueCallInfo *pst_rule = NULL;

    if( (pdoc=xmlReadFile (pc_file_path, "UTF-8", XML_PARSE_RECOVER))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call xmlReadFile(%s) function failed", pc_file_path);
        return -1;    
    }
    //get root node
    if( (pnode_root=xmlDocGetRootElement(pdoc))==NULL )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "call xmlDocGetRootElement(doc: %s) function failed", pc_file_path);
        xmlFreeDoc (pdoc);
        return -1;
    }
    if( xmlStrcmp(pnode_root->name, BAD_CAST"CLUE_INFO")!=0 )
    {
        applog (APP_LOG_LEVEL_NOTICE, APP_LOG_MASK_RFU, "in the %s rule file root node name not is <CLUE_INFO>", pc_file_path);
        xmlFreeDoc (pdoc);
        return -1;
    }
    
    pnode_root = pnode_root->xmlChildrenNode;
    while( pnode_root!=NULL )
    {
        pst_rule = (ClueCallInfo *)ods_malloc(sizeof(ClueCallInfo));
        if( das_rule_save_to_stuct(pnode_root, pst_rule)==0 )
        {
            (*pf_opt_fun)(pst_rule);
        }
        else
        {
            ods_free(pst_rule);
        }

        pnode_root = pnode_root->next;
    }

    xmlFreeDoc (pdoc);
    
    return 0;
}

int das_timeout(unsigned int tv)
{
    static time_t last_tv = 0;
    time_t tmp_tv = 0;

    if( gi_link_signal_flg==0 && tv%3==0 ) 
    {//link das signal port, qeust gloabal 
        int fd = -1;
        if( (fd=connect2srv(gst_conf.ac_das_ip, gst_conf.das_signal_port))<0 )
        {
            return -1;    
        }
        else
        {
            close (fd);
            gi_link_signal_flg = 1;
        }
    }
    
    //定时请求全量,time是1个小时
    tmp_tv = time(NULL);
    if( last_tv!=0 && tmp_tv-last_tv>3600 )
    {
        last_tv = tmp_tv;
        gi_link_signal_flg = 0;
    }
    if( last_tv==0 )
    {
        last_tv = time(NULL);
    }

    if( tv%20==0 )
    {
        if( parse_rule_error_num>0 )
        {
            applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "parse das issue rule format error of packate numbler: %u!", parse_rule_error_num);
        }
    }

    return 0;
}

int das_add_detect_fd (fd_set *rfds)
{
    int i;
    if( rfds==NULL )
    {
        applog (APP_LOG_LEVEL_INFO, APP_LOG_MASK_RFU, "call %s function failed, argument error!", __FUNCTION__ );
        return -1;
    }
    if( gi_das_listen_fd>0 )
    {
        FD_SET (gi_das_listen_fd, rfds);
    }
    if( gi_cfu_listen_fd>0 )
    {
        FD_SET (gi_cfu_listen_fd, rfds);
    }
    for( i=0; i<sizeof(gai_das_link_fd)/sizeof(gai_das_link_fd[0]); ++i )
    {
        if( gai_das_link_fd[i]>0 )
        {
            FD_SET (gai_das_link_fd[i], rfds);
        }
    }

    return 0;
}

void das_rule_save_callback(ClueCallInfo *pst_src)
{
    read_svm_local_fail_rule (pst_src);            
    read_wireless_local_fail_rule (pst_src);

    //das的规则
    container_add_data (das_rule_save, pst_src);
}

int das_init()
{
    open_das_link_port (gst_conf.das_data_port);
    open_cfu_link_port (gst_conf.cfu_listen_port);
    das_rule_save = create_container ();        
    
    char ac_path[500] = {0};
    snprintf (ac_path, sizeof(ac_path), "%s%s", gst_conf.ac_conf_path_base, DAS_RULE_SAVE_FILE_NAME);
    
    //read bunnies software local rule
    svm_read_local_success_rule ();    
    wireless_read_local_success_rule ();

    //read das local rule
    read_rule_xml_file (ac_path, das_rule_save_callback);
    
    wireless_svm_init ();    
    
    return 0;
}

unsigned int get_das_total_rule()
{
    return container_node_num(das_rule_save);
}


