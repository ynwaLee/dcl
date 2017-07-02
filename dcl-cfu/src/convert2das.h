#ifndef __CONVERT2DAS_H__ 
#define __CONVERT2DAS_H__ 

#include "svm_cdr.h"
#include "mobile_cdr.h"
#include "voipa_cdr.h"
#include "voipb_cdr.h"


typedef struct das_record_{
    char id[16];
    char clue_id[16];
    char base_station_id[64];
    char location_id[64];
    char ci[64];
    char station_type[16];
    char imsi[64];
    char bhome[64];
    char imei[128];
    char apc[64];
    char calling_num[64];
    char called_num[64];
    char action[4];
    char call_type[4];
    char upareaid[16];
    char data_source[16];
    char capture_time[16];
    char starttime[16];
    char endtime[16];
    char lasttime[16];
    char isp_id[16];
    char local_bsattribution[64];
    char nonlocal_bsattribution[64];
    char phoneattribution[64];
    char n_phoneattribution[64];
    char tmsi[32];
    char longitude[32];
    char latitude[32];
    char alacs[128];
    char acells[128];
    char alace[128];
    char acelle[128];
    char btisdn[64];
    char vocfile[256];
    char extinfo[64];
    char result[64];
    char cause[64];
    char ispid[64];
    char mntid[64];
    char smslan[64];
    char smstext[2048];
    char callid[32];
    char callflag[4];
    char cbid1[16];
    char cbid2[16];
    char cbid3[16];
    char score1[16];
    char score2[16];
    char score3[16];
    char voice_id1[16];
    char voice_id2[16];
    char voice_id3[16];
    char remarkstr[255];
    char c1[255];
    char c2[255];
    char i1[16];
    char i2[16];
    char object_id[16];
    char ahome[64];
    char aneid[64];
    char amcc[16];
    char amnc[16];
    char bisdnx[64];
    char bimsi[64];
    char bneid[64];
    char bpc[64];
    char linklayerid[16];
    char inunit[16];
    char outunit[16];
    char path[16];
    char systype[16];
    char online_flag[16];
    char callcode[16];
} das_record_t;


#if 0
typedef struct voip_record_{
    char userid[12];
    char usertype[128];
    char location[32];
    char application[16];
    char appid[128];
    char nbdomain[128];
    char behavior[512];
    char authtype[16];
    char authaccount[128];
    char period[16];
    char nbdatasource[16];
    char time[16];
    char nbkeyword[1024];
    char nbkeywordid[1024];
    char terminalid[1024];
    char terminalname[1024];
    char tgdid[1024];
    char nbstore[1024];
    char trashruleid[1024];
    char objectid[1024];
    char id[16]; //NOT NULL
    char clue_id[16];
    char upareaid[16];
    char clue_src_sys[64];
    char clue_dst_sys[64];
    char isp_id[12];
    char device_id[128];
    char lineid[16];
    char src_ip[16];
    char dst_ip[16];
    char src_ipid[16];
    char dst_ipid[16];
    char strsrc_ip[64];
    char strdst_ip[64];
    char bsattribution[64];
    char phoneattribution[64];
    char teid[64];
    char src_port[16];
    char dst_port[16];
    char mac[64];
    char capture_time[16]; //NOT NULL
    char data_source[12];
    char company_name[128];
    char country_type[64];
    char certificate_code[128];
    char certificate_type[12];
    char sessionid[64];
    char auth_type[12];
    char relatedirection[16];
    char auth_account[64];
    char imsi[64];
    char equipment_id[128];
    char hardware_signature[64];
    char servicecode[32];
    char base_station_id[64];
    char context[1024];
    char longitude[64];
    char latitude[64];
    char user_agent[255];
    char tooltype[12];
    char toolname[64];
    char voip_type[12]; // NOT NULL
    char chatroom[128];
    char username[128];
    char password[64];
    char nickname[255];
    char from_id[64];
    char to_id[128];
    char action[12];
    char content[4000];
    char mainfile[4000];
    char filesize[16];
    char audiofile1[255];
    char audiofile2[255];
    char videofile1[255];
    char videofile2[255];
    char itemflag[64];
    char recordflag[16];
    char superclass[64];
    char subclass[64];
    char remarkstr[255];
    char c1[255];
    char c2[255];
    char i1[16];
    char i2[16];
    char roomid[64];
    char src_auth_account[64];
    char src_certificate_code[128];
    char datalevel[32];
    char activeareaid[32];
    char vlanid[64];
    char four_station_id[64];
    char ip_type[16];
    char src_ipv6[16];
    char dst_ipv6[16];
    char strsrc_ipv6[64];
    char strdst_ipv6[64];
    char src_ipidv6[16];
    char dst_ipidv6[16];
    char bitaction[16];
} voip_record_t;
#else
typedef struct voip_record_{
    char capture_time[16];
    char data_source[8];
    char company_name[16];
    char collect_place[32];
    char app_layer_protocol[32];
    char record_id[16];
    char voip_type[16];
    char from_id[32];
    char to_id[32];
    char action[16];
    char sender_phone[32];
    char receiver_phone[32];
    char call_starttime[16];
    char call_usedtime[16];
} voip_record_t;
#endif



extern int mass_session;
extern int case_session;


int svm2das_mass_voice(voice_record_t *svm_record, das_record_t *das_record);
int svm2das_case_voice(voice_record_t *svm_record, das_record_t *das_record, unsigned int object_id);
int svm2das_mass_sms(sms_record_t *svm_record, das_record_t *das_record);
int svm2das_case_sms(sms_record_t *svm_record, das_record_t *das_record, unsigned int object_id);


int mobile2das_mass(mobile_mass_t *mass, das_record_t *das);
int mobile2das_case(mobile_case_t *cases, das_record_t *das, unsigned int object_id);

int voipa2das_mass(voipa_mass_t *mass, voip_record_t *das);
int voipa2das_case(voipa_case_t *cases, das_record_t *das, unsigned int object_id);

int voipb2das_mass(voipb_mass_t *mass, das_record_t *das);
int voipb2das_case(voipb_case_t *cases, das_record_t *das, unsigned int object_id);


int write_bcp(FILE *fp, das_record_t *das_record);
int write_voip_bcp(FILE *fp, voip_record_t *voip_record);


#endif
