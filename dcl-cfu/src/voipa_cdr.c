#define _XOPEN_SOURCE   600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/inotify.h>
#include "libxml/parser.h"
#include "libxml/xmlmemory.h"
#include "libxml/tree.h"
#include "atomic.h"
#include "cfu.h"
#include "conf_parser.h"
#include "svm_cdr.h"
#include "voipa_cdr.h"
#include "convert2das.h"
#include "file_fifo.h"
#include "ftplib.h"
#include "rfu_client.h"
#include "counter.h"
#include "mass_ftp_upload.h"
#include "case_ftp_upload.h"
#include "../../common/applog.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"
#include "voipa_cdr_filter.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 64 ) )
#define VOIPA_DIR   "voip-a"


#define DIR_TMP "/dev/shm"
#define BCP_XML_FILE "/dev/shm/GAB_ZIP_INDEX.xml"



extern atomic32_t mass_bcp_num;
extern atomic32_t case_bcp_num;






int check_voipa_dir(void)
{
    struct stat dir_stat;
    char cmd[1024];
    char path[1024];

    snprintf(path, 1023, "%s/%s", cfu_conf.local_ftp_dir, VOIPA_DIR);

    if (stat(path, &dir_stat) != 0 || (dir_stat.st_mode & S_IFMT) != S_IFDIR) {
        snprintf(cmd, 1024, "mkdir -p %s", path);
        system(cmd);
    }

    snprintf(cmd, 1024, "chown -R %s %s", cfu_conf.local_ftp_user, path);
    system(cmd);
    snprintf(cmd, 1024, "chmod -R 777 %s", path);
    system(cmd);

    return 0;
}






int voipa_mass_parser(char *buf, voipa_mass_t *mass)
{
    char *p;
    char *q;
    int n;

    p = buf;
    n = 0;
    memset(mass, 0, sizeof(voipa_mass_t));
    while (1) {
        q = strchr(p, ',');
        if (q == NULL) {
            break;
        }
        n++;
        switch (n) {
            case 1:
                strncpy(mass->aisdn, p + 1, q - p - 2);
                break;
            case 2:
                strncpy(mass->ahome, p + 1, q - p - 2);
                break;
            case 3:
                strncpy(mass->aimsi, p + 1, q - p - 2);
                break;
            case 4:
                strncpy(mass->aimei, p + 1, q - p - 2);
                break;
            case 5:
                strncpy(mass->aneid, p + 1, q - p - 2);
                break;
            case 6:
                strncpy(mass->amcc, p + 1, q - p - 2);
                break;
            case 7:
                strncpy(mass->amnc, p + 1, q - p - 2);
                break;
            case 8:
                strncpy(mass->alacs, p + 1, q - p - 2);
                break;
            case 9:
                strncpy(mass->acells, p + 1, q - p - 2);
                break;
            case 10:
                strncpy(mass->alace, p + 1, q - p - 2);
                break;
            case 11:
                strncpy(mass->acelle, p + 1, q - p - 2);
                break;
            case 12:
                strncpy(mass->apc, p + 1, q - p - 2);
                break;
            case 13:
                strncpy(mass->bisdn, p + 1, q - p - 2);
                break;
            case 14:
                strncpy(mass->bisdnx, p + 1, q - p - 2);
                break;
            case 15:
                strncpy(mass->bhome, p + 1, q - p - 2);
                break;
            case 16:
                strncpy(mass->btisdn, p + 1, q - p - 2);
                break;
            case 17:
                strncpy(mass->bimsi, p + 1, q - p - 2);
                break;
            case 18:
                strncpy(mass->bneid, p + 1, q - p - 2);
                break;
            case 19:
                strncpy(mass->bpc, p + 1, q - p - 2);
                break;
            case 20:
                strncpy(mass->acttype, p + 1, q - p - 2);
                break;
            case 21:
                strncpy(mass->starttime, p + 1, q - p - 2);
                break;
            case 22:
                strncpy(mass->endtime, p + 1, q - p - 2);
                break;
            case 23:
                strncpy(mass->duration, p + 1, q - p - 2);
                break;
            case 24:
                strncpy(mass->vocfile, p + 1, q - p - 2);
                break;
            case 25:
                strncpy(mass->extinfo, p + 1, q - p - 2);
                break;
            case 26:
                strncpy(mass->result, p + 1, q - p - 2);
                break;
            case 27:
                strncpy(mass->cause, p + 1, q - p - 2);
                break;
            case 28:
                strncpy(mass->netid, p + 1, q - p - 2);
                break;
            case 29:
                strncpy(mass->smslan, p + 1, q - p - 2);
                break;
            case 30:
                strncpy(mass->smstext, p + 1, q - p - 2);
                break;
        }
        p = q + 1;
    }
    if (n == 29) {
        strncpy(mass->smstext, p + 1, strlen(p) - 4);
        n++;
    }

    return n;
}



int voipa_case_parser(char *buf, voipa_case_t *cases)
{
    char *p;
    char *q;
    int n;

    p = buf;
    n = 0;
    memset(cases, 0, sizeof(voipa_case_t));
    while (1) {
        q = strchr(p, ',');
        if (q == NULL) {
            break;
        }
        n++;
        switch (n) {
            case 1:
                strncpy(cases->mntid, p + 1, q - p - 2);
                break;
            case 2:
                strncpy(cases->aisdn, p + 1, q - p - 2);
                break;
            case 3:
                strncpy(cases->ahome, p + 1, q - p - 2);
                break;
            case 4:
                strncpy(cases->aimsi, p + 1, q - p - 2);
                break;
            case 5:
                strncpy(cases->aimei, p + 1, q - p - 2);
                break;
            case 6:
                strncpy(cases->aneid, p + 1, q - p - 2);
                break;
            case 7:
                strncpy(cases->amcc, p + 1, q - p - 2);
                break;
            case 8:
                strncpy(cases->amnc, p + 1, q - p - 2);
                break;
            case 9:
                strncpy(cases->alacs, p + 1, q - p - 2);
                break;
            case 10:
                strncpy(cases->acells, p + 1, q - p - 2);
                break;
            case 11:
                strncpy(cases->alace, p + 1, q - p - 2);
                break;
            case 12:
                strncpy(cases->acelle, p + 1, q - p - 2);
                break;
            case 13:
                strncpy(cases->apc, p + 1, q - p - 2);
                break;
            case 14:
                strncpy(cases->bisdn, p + 1, q - p - 2);
                break;
            case 15:
                strncpy(cases->bisdnx, p + 1, q - p - 2);
                break;
            case 16:
                strncpy(cases->bhome, p + 1, q - p - 2);
                break;
            case 17:
                strncpy(cases->btisdn, p + 1, q - p - 2);
                break;
            case 18:
                strncpy(cases->bimsi, p + 1, q - p - 2);
                break;
            case 19:
                strncpy(cases->bneid, p + 1, q - p - 2);
                break;
            case 20:
                strncpy(cases->bpc, p + 1, q - p - 2);
                break;
            case 21:
                strncpy(cases->acttype, p + 1, q - p - 2);
                break;
            case 22:
                strncpy(cases->starttime, p + 1, q - p - 2);
                break;
            case 23:
                strncpy(cases->endtime, p + 1, q - p - 2);
                break;
            case 24:
                strncpy(cases->duration, p + 1, q - p - 2);
                break;
            case 25:
                strncpy(cases->vocfile, p + 1, q - p - 2);
                break;
            case 26:
                strncpy(cases->extinfo, p + 1, q - p - 2);
                break;
            case 27:
                strncpy(cases->result, p + 1, q - p - 2);
                break;
            case 28:
                strncpy(cases->cause, p + 1, q - p - 2);
                break;
            case 29:
                strncpy(cases->netid, p + 1, q - p - 2);
                break;
            case 30:
                strncpy(cases->smslan, p + 1, q - p - 2);
                break;
        }
        p = q + 1;
    }
    if (n == 30) {
        strncpy(cases->smstext, p + 1, strlen(p) - 4);
        n++;
    }

    return n;
}




inline int first_item_newnode(xmlNodePtr *node, char *key, char *val, char *rmk)
{
    xmlNodePtr pnode_child;

    if (node == NULL || key == NULL || val == NULL || rmk == NULL) {
        return -1;
    }

    pnode_child = xmlNewNode(NULL, BAD_CAST"ITEM");
    xmlNewProp(pnode_child, BAD_CAST"key", (xmlChar *)key);
    xmlNewProp(pnode_child, BAD_CAST"val", (xmlChar *)val);
    xmlNewProp(pnode_child, BAD_CAST"rmk", (xmlChar *)rmk);
    xmlAddChild(*node, pnode_child);

    return 0;
}


inline int second_item_newnode(xmlNodePtr *node, char *key, char *eng, char *chn)
{
    xmlNodePtr pnode_child;

    if (node == NULL || key == NULL || eng == NULL || chn == NULL) {
        return -1;
    }

    pnode_child = xmlNewNode(NULL, BAD_CAST"ITEM");
    xmlNewProp(pnode_child, BAD_CAST"key", (xmlChar *)key);
    xmlNewProp(pnode_child, BAD_CAST"eng", (xmlChar *)eng);
    xmlNewProp(pnode_child, BAD_CAST"chn", (xmlChar *)chn);
    xmlAddChild(*node, pnode_child);

    return 0;
}




int make_bcp_xml(char *filename, unsigned int line)
{
    xmlDocPtr pdoc;
    xmlNodePtr pnode_root;
    xmlNodePtr pnode_child_1;
    xmlNodePtr pnode_child_2;
    xmlNodePtr pnode_child_3;
    xmlNodePtr pnode_child_4;
    xmlNodePtr pnode_child_5;
    xmlNodePtr pnode_child_6;
    char line_num[16];

    if (filename == NULL || line <= 0) {
        return -1;
    }

    if((pdoc = xmlNewDoc(BAD_CAST"1.0")) == NULL) {
        printf("xmlNewDoc failed\n");
        return -1;
    }

    pnode_root = xmlNewNode(NULL, BAD_CAST"MESSAGE");
    xmlDocSetRootElement(pdoc, pnode_root);

    pnode_child_1 = xmlNewNode(NULL, BAD_CAST"DATASET");
    xmlNewProp(pnode_child_1, BAD_CAST"name", (xmlChar *)"WA_COMMON_010017");
    xmlNewProp(pnode_child_1, BAD_CAST"rmk", (xmlChar *)"数据文件索引信息");
    xmlAddChild(pnode_root, pnode_child_1);

    pnode_child_2 = xmlNewNode(NULL, BAD_CAST"DATA");
    xmlAddChild(pnode_child_1, pnode_child_2);

    pnode_child_3 = xmlNewNode(NULL, BAD_CAST"DATASET");
    xmlNewProp(pnode_child_3, BAD_CAST"name", (xmlChar *)"WA_COMMON_010013");
    xmlNewProp(pnode_child_3, BAD_CAST"rmk", (xmlChar *)"BCP文件格式信息");
    xmlAddChild(pnode_child_2, pnode_child_3);

    pnode_child_4 = xmlNewNode(NULL, BAD_CAST"DATA");
    xmlAddChild(pnode_child_3, pnode_child_4);

    first_item_newnode(&pnode_child_4, "I010032", "\\t", "列分隔符（缺少值时默认为制表符\\t）");
    first_item_newnode(&pnode_child_4, "I010033", "\\n", "行分隔符（缺少值时默认为换行符\\n）");
    first_item_newnode(&pnode_child_4, "A010004", "WA_SOURCE_0009", "数据集代码");
    first_item_newnode(&pnode_child_4, "B050016", "111", "数据来源");
    //first_item_newnode(&pnode_child_4, "F010008", "440000", "数据采集地");
    first_item_newnode(&pnode_child_4, "F010008", "320100", "数据采集地");
    first_item_newnode(&pnode_child_4, "I010038", "1", "数据起始行，可选项，不填写默认为第１行");
    first_item_newnode(&pnode_child_4, "I010039", "UTF-8", "可选项，默认为UTF-8，BCP文件编码格式（采用不带格式的编码方式，如：UTF-8无BOM）");

    pnode_child_5 = xmlNewNode(NULL, BAD_CAST"DATASET");
    xmlNewProp(pnode_child_5, BAD_CAST"name", (xmlChar *)"WA_COMMON_010015");
    xmlNewProp(pnode_child_5, BAD_CAST"rmk", (xmlChar *)"BCP文件数据结构");
    xmlAddChild(pnode_child_4, pnode_child_5);

    pnode_child_6 = xmlNewNode(NULL, BAD_CAST"DATA");
    xmlAddChild(pnode_child_5, pnode_child_6);

    second_item_newnode(&pnode_child_6, "H010014", "CAPTURE_TIME", "截获时间(上线时间)");
    second_item_newnode(&pnode_child_6, "B050016", "DATA_SOURCE", "数据来源名称");
    second_item_newnode(&pnode_child_6, "G020014", "COMPANY_NAME", "厂商名称");
    second_item_newnode(&pnode_child_6, "F010008", "COLLECT_PLACE", "采集地");
    second_item_newnode(&pnode_child_6, "H010041", "APPLICATION_LAYER_PROTOCOL", "应用层协议");
    second_item_newnode(&pnode_child_6, "H010042", "RECORD_ID", "记录ID");
    second_item_newnode(&pnode_child_6, "H010001", "VOIP_TYPE", "协议代码");
    second_item_newnode(&pnode_child_6, "B050004", "FROM_ID", "发送帐号或电话");
    second_item_newnode(&pnode_child_6, "B050009", "TO_ID", "接收帐号或电话");
    second_item_newnode(&pnode_child_6, "H010003", "ACTION", "操作类型");
    second_item_newnode(&pnode_child_6, "B050023", "SENDER_PHONE", "主叫方电话号码");
    second_item_newnode(&pnode_child_6, "B050024", "RECEIVER_PHONE", "被叫方电话号码");
    second_item_newnode(&pnode_child_6, "H040008", "CALL_STARTTIME", "呼叫发起时间");
    second_item_newnode(&pnode_child_6, "H010030", "CALL_USEDTIME", "通话时长");


    pnode_child_5 = xmlNewNode(NULL, BAD_CAST"DATASET");
    xmlNewProp(pnode_child_5, BAD_CAST"name", (xmlChar *)"WA_COMMON_010014");
    xmlNewProp(pnode_child_5, BAD_CAST"rmk", (xmlChar *)"BCP数据文件信息");
    xmlAddChild(pnode_child_4, pnode_child_5);

    pnode_child_6 = xmlNewNode(NULL, BAD_CAST"DATA");
    xmlAddChild(pnode_child_5, pnode_child_6);

    first_item_newnode(&pnode_child_6, "H040003", "./", "文件路径");
    first_item_newnode(&pnode_child_6, "H010020", filename, "文件名");
    snprintf(line_num, 16, "%u", line);
    first_item_newnode(&pnode_child_6, "I010034", line_num, "记录行数");

    xmlSaveFormatFileEnc(BCP_XML_FILE, pdoc, "UTF-8", 1);
    xmlFreeDoc(pdoc);

    return 0;
}

int voipa_mass_convert(char *path, char *filename)
{
    FILE *fp_src;
    FILE *fp_dst;
    char buffer[1024] = {0};
    int return_value;
    int bcp_num;
    st_voipa_cdr_t voipa_cdr;
    voip_record_t voip_record;
    file_node_t file_node;
    char file_local[FILENAME_LEN];
    char filebackup_local[FILENAME_LEN];
    char filetransfer_tmp[FILENAME_LEN];
    char filename_bcp[FILENAME_LEN];
    char filename_bcp_tmp[FILENAME_LEN];
    char zip_cmd[FILENAME_LEN];
    struct tm *tms;
    struct tm tms_buf;
    int cdr_counter;
	int src_city_code_len = 0;
	int dec_city_code_len = 0;
	int nret = 0;

    if (path == NULL || filename == NULL) {
        return -1;
    }

	src_city_code_len = strlen(cfu_conf.src_city_code);
	dec_city_code_len = strlen(cfu_conf.dec_city_code);

    snprintf(file_local, FILENAME_LEN, "%s/%s", path, filename);
    bcp_num = atomic32_add_return(&mass_bcp_num, 1);
    if (bcp_num > 99999) {
        atomic32_init(&mass_bcp_num);
        bcp_num = atomic32_add_return(&mass_bcp_num, 1);
    }
    //snprintf(filename_bcp_tmp, FILENAME_LEN, "111-440100-%u-%05u-WA_SOURCE_0009-0.bcp", (unsigned int)g_time, bcp_num);
    if(src_city_code_len == 0)   //没有城市码
    {
        snprintf(filename_bcp_tmp, FILENAME_LEN, "111-000000-%u-%05u-WA_SOURCE_0009-0.bcp", (unsigned int)g_time, bcp_num);
    }
	else
	{
	    snprintf(filename_bcp_tmp, FILENAME_LEN, "111-%s-%u-%05u-WA_SOURCE_0009-0.bcp", cfu_conf.src_city_code , (unsigned int)g_time, bcp_num);
	}
	snprintf(filename_bcp, FILENAME_LEN, "%s/%s", DIR_TMP, filename_bcp_tmp);

    memset(&file_node, 0, sizeof(file_node_t));
    //snprintf(file_node.filename, 256, "111-831196-440100-440100-%u-%05u.zip", (unsigned int)g_time, bcp_num);
    snprintf(file_node.filename, 256, "111-831196-%s-%s-%u-%05u.zip", ((src_city_code_len == 0)?("000000"):(cfu_conf.src_city_code)),
                                                                       ((dec_city_code_len == 0)?("000000"):(cfu_conf.dec_city_code)),
                                                                       (unsigned int)g_time, bcp_num);
	
    snprintf(file_node.file_transfer, 512, "%s/%s/%s", cfu_conf.local_transfer_dir, ZIP_DIR, file_node.filename);
    snprintf(filetransfer_tmp, FILENAME_LEN, "%s/%s/tmp-%s", cfu_conf.local_transfer_dir, ZIP_DIR, file_node.filename);
    file_node.case_flag = 0;

    fp_src = fopen(file_local, "r");
    if (fp_src == NULL) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", file_local, strerror(errno));
        return -1;
    }
    fp_dst = fopen(filename_bcp, "w+");
    if (fp_dst == NULL) {
        fclose(fp_src);
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", filename_bcp, strerror(errno));
        return -1;
    }

    cdr_counter = 0;
    while(fgets(buffer, 1024, fp_src) != NULL) {
        return_value = voipa_mass_parser(buffer, &voipa_cdr.voip_mass);
		
        if (return_value == 30) 
        {
            if(cfu_conf.voip_filter_enable && 
                (filter_voipa_cdr(&voipa_cdr, VOIPA_TYPE_MASS) == 0))
            {
                applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE,
                        "[MASS CDR DROP] acttype: %s, A ISDN: %s, B ISDN: %s",
                        voipa_cdr.voip_mass.acttype,
                        voipa_cdr.voip_mass.aisdn,
                        voipa_cdr.voip_mass.bisdn);
                continue;
            }
            
            atomic64_add(&voipa_mass_call_num, 1);
			
            voipa2das_mass(&voipa_cdr.voip_mass, &voip_record);
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "[MASS CDR FORWARD] mass actype: %s, call_id: %s, called_id: %s !", voipa_cdr.voip_mass.acttype,  voip_record.from_id , voip_record.to_id);
            nret = write_voip_bcp(fp_dst, &voip_record);
            if(nret != 0)
            {
                applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "action = %s from_id=%s to_id=%s filter", voip_record.action , voip_record.from_id , voip_record.to_id);
                continue;
            }
            atomic64_add(&das_mass_call_num, 1);
            cdr_counter += 1;
        }
    }
    //applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "voipa mass convert %d cdr", cdr_counter);

    fclose(fp_src);
    fclose(fp_dst);
    
    //备份CSV话单
    tms = localtime_r(&g_time, &tms_buf);
    if (tms == NULL) {
        return -1;
    }
    snprintf(filebackup_local, FILENAME_LEN, "%s/%04d-%02d-%02d/%02d/%s", 
            cfu_conf.local_backup_dir, tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, filename);
    rename(file_local, filebackup_local);

    //没有一条话单记录，返回
    if(cdr_counter == 0)
    {
        remove(filename_bcp);
        return 0;
    }

	//打包BCP和XML
    make_bcp_xml(filename_bcp_tmp, cdr_counter);
    snprintf(zip_cmd, FILENAME_LEN, "zip -j %s %s %s", filetransfer_tmp, filename_bcp, BCP_XML_FILE);
    system(zip_cmd);
    remove(filename_bcp);
    remove(BCP_XML_FILE);
    rename(filetransfer_tmp, file_node.file_transfer);

    atomic64_add(&das_mass_file_num, 1);

    return 0;
}


int voipa_case_convert(char *path, char *filename)
{
    FILE *fp_src;
    FILE *fp_dst;
    char buffer[1024] = {0};
    int return_value;
    int bcp_num;
    int object_id;
    int clue_id;
    uint32_t random_num;
    st_voipa_cdr_t voipa_cdr;
    das_record_t das_record;
    file_node_t file_node;
    char file_local[FILENAME_LEN];
    char filebackup_local[FILENAME_LEN];
    char filetransfer_tmp[FILENAME_LEN];
    struct tm *tms;
    struct tm tms_buf;
    int cdr_counter;
	int nret = 0;

    if (path == NULL || filename == NULL) {
        return -1;
    }
    snprintf(file_local, FILENAME_LEN, "%s/%s", path, filename);

    fp_src = fopen(file_local, "r");
    if (fp_src == NULL) {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", file_local, strerror(errno));
        return -1;
    }

    cdr_counter = 0;
    while(fgets(buffer, 1024, fp_src) != NULL) {
        return_value = voipa_case_parser(buffer, &voipa_cdr.voip_case);
        if (return_value != 31) {
            continue;
        }

        if(cfu_conf.voip_filter_enable && 
            (filter_voipa_cdr(&voipa_cdr, VOIPA_TYPE_CASE) == 0))
        {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE,
                    "[CASE CDR DROP] mntid: %s, acttype: %s, A ISDN: %s, B ISDN: %s",
                    voipa_cdr.voip_case.mntid,
                    voipa_cdr.voip_case.acttype,
                    voipa_cdr.voip_case.aisdn,
                    voipa_cdr.voip_case.bisdn);
            continue;
        }

        atomic64_add(&voipa_case_call_num, 1);
        clue_id = atoi(voipa_cdr.voip_case.mntid) - 200;
        object_id = get_object(clue_id);
        if (object_id == 0) {
            applog(APP_LOG_LEVEL_WARNING, APP_LOG_MASK_TELE,
                    "object_id is not found, clue_id : %d", clue_id);
            continue;
        }
        
        memset(&file_node, 0, sizeof(file_node_t));
        bcp_num = atomic32_add_return(&case_bcp_num, 1);
        random_num = (g_time << 16) | (bcp_num & 0xffff);
        snprintf(file_node.filename, 256, "CASE_%u_CALL_%u.bcp", random_num, object_id);
        snprintf(filetransfer_tmp, 512, "%s/%s/%s.tmp", cfu_conf.local_transfer_dir, CASE_DIR, file_node.filename);
        snprintf(file_node.file_transfer, 512, "%s/%s/%s", cfu_conf.local_transfer_dir, CASE_DIR, file_node.filename);
        file_node.case_flag = 1;
        
        fp_dst = fopen(filetransfer_tmp, "w+");
        if (fp_dst == NULL) {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_TELE, "fopen %s error, %s", filetransfer_tmp, strerror(errno));
            continue;
        }

        voipa2das_case(&voipa_cdr.voip_case, &das_record, object_id);
		
        nret = write_bcp(fp_dst, &das_record);
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "[CASE CDR FORWARD] write ret = %d call_id=%s to_id=%s", nret , das_record.calling_num , das_record.called_num);
        if(nret != 0)
        {
            fclose(fp_dst);
            continue;
        }
		
        atomic64_add(&das_case_call_num, 1);
        cdr_counter += 1;

        fclose(fp_dst);
        rename(filetransfer_tmp, file_node.file_transfer);
        atomic64_add(&das_case_file_num, 1);
    }
    //applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "voipa case convert %d cdr", cdr_counter);

    fclose(fp_src);
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_TELE, "case cdr_counter = %d ", cdr_counter);
	
    //backup file
    tms = localtime_r(&g_time, &tms_buf);
    if (tms == NULL) {
        return -1;
    }
    snprintf(filebackup_local, FILENAME_LEN, "%s/%04d-%02d-%02d/%02d/%s", 
            cfu_conf.local_backup_dir, tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, filename);
    rename(file_local, filebackup_local);
    
    return 0;
}




int settle_voipa_history_file(void)
{
    char path[1024];
    DIR *fp;
    struct dirent dir;
    struct dirent *dirp = NULL;
    char *offset;

    snprintf(path, 1023, "%s/%s", cfu_conf.local_ftp_dir, VOIPA_DIR);

    fp = opendir(path);
    if (fp == NULL) {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "open %s failed", path);
        return -1;
    }

    while (readdir_r(fp, &dir, &dirp) == 0 && dirp != NULL) {
        if (dir.d_name[0] == '.') {
            continue;
        }
        if (check_filetype(dir.d_name, ".csv") == 0) {
            offset = strchr(dir.d_name, '.');
            if (offset != NULL) {
                if (offset - dir.d_name == 12) {
                    //mass
                    voipa_mass_convert(path, dir.d_name);
                    atomic64_add(&voipa_mass_file_num, 1);
                } else if (offset - dir.d_name == 14) {
                    //case
                    voipa_case_convert(path, dir.d_name);
                    atomic64_add(&voipa_case_file_num, 1);
                }
            }
        }
    }

    closedir(fp);

    return 0;
}




void *thread_voipa_cdr(void *arg)
{
    char voipa_dir[1024];
    char *offset;
    DIR *fp_dir = NULL;
    struct dirent dir;
    struct dirent *dirp = NULL;
    //struct timeval tv1,tv2;
    //struct timezone tz1,tz2;


    check_voipa_dir();

    // 初始化voip话单过滤组件
    if(cfu_conf.voip_filter_enable)
    {
        if(init_voipa_cdr_filter() < 0)
        {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "[%s:%d] init voipa cdr filter failed", __FILE__, __LINE__);
            applog(APP_LOG_LEVEL_WARNING, APP_LOG_MASK_BASE, "[%s:%d] turn-off voip_filter_enable", __FILE__, __LINE__);
            cfu_conf.voip_filter_enable = 0;
        }
    }

    snprintf(voipa_dir, 1023, "%s/%s", cfu_conf.local_ftp_dir, VOIPA_DIR);

    while (1) {
        if (unlikely(svm_signal_flags & (THREAD_RUN_KILL | THREAD_RUN_STOP))) {
            break;
        }

        if (fp_dir == NULL) {
            fp_dir = opendir(voipa_dir);
            if (fp_dir == NULL) {
                applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "open %s failed, %s", voipa_dir, strerror(errno));
                continue;
            }
        }

        if (readdir_r(fp_dir, &dir, &dirp) != 0) {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "read %s error, %s", voipa_dir, strerror(errno));
            closedir(fp_dir);
            fp_dir = NULL;
            continue;
        }
        if (dirp == NULL) {
            rewinddir(fp_dir);
            usleep(1);
            continue;
        }
        if (dir.d_name[0] == '.') {
            continue;
        }
        
        if (check_filetype(dir.d_name, ".csv") == 0) {
            offset = strchr(dir.d_name, '.');
            if (offset != NULL) {
                if (offset - dir.d_name == 12) {
                    //gettimeofday(&tv1,&tz1);
                    //mass
                    voipa_mass_convert(voipa_dir, dir.d_name);
                    atomic64_add(&voipa_mass_file_num, 1);
                    //gettimeofday(&tv2,&tz2);
                    //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "voipa mass cdr convert to bcp spent %lu usec.", 
                            //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
                } else if ((offset - dir.d_name == 15) || (offset - dir.d_name == 14)) {
                    //gettimeofday(&tv1,&tz1);
                    //case
                    voipa_case_convert(voipa_dir, dir.d_name);
                    atomic64_add(&voipa_case_file_num, 1);
                    //gettimeofday(&tv2,&tz2);
                    //applog(LOG_INFO, APP_LOG_MASK_SIGNAL, "voipa case cdr convert to bcp spent %lu usec.", 
                            //(tv2.tv_usec>tv1.tv_usec) ? (tv2.tv_usec-tv1.tv_usec):(1000000+tv2.tv_usec-tv1.tv_usec));
                } else {
                    //error
                    applog(APP_LOG_LEVEL_DEBUG, APP_LOG_MASK_TELE, 
                            "voipa file error: %s", dir.d_name);
                }
            }
        }

    }

    closedir(fp_dir);

    destory_voipa_cdr_filter();
    pthread_exit(NULL);
}
