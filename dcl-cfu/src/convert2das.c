#define _XOPEN_SOURCE   600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "atomic.h"
#include "cfu.h"
#include "svm_cdr.h"
#include "convert2das.h"
#include "../../common/applog.h"

#include "conf_parser.h"




atomic32_t das_session;


#define CALL_VOICE  2000001
#define CALL_SMS    2000002
#define CALL_FAX    2000003
#define CALL_OTHER  2000004

//号码归一化，话单中的号码格式应当和svm一致。使用信令中的原始号码且不带"+"号
//如果YJ生成的话单里带了"+"号，就跳过它
#define COPY_ISDN(dst, src, maxlen)     strncpy(dst, src + (src[0] == '+' ? 1 : 0), maxlen)

int svm2das_mass_voice(voice_record_t *svm, das_record_t *das)
{
    unsigned int id;

    memset(das, 0, sizeof(das_record_t));

    id = atomic32_add_return(&das_session, 1);
    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);
    strncpy(das->systype, "1", 1);
    strncpy(das->data_source, "111", 3);
    strncpy(das->isp_id, "01", 2);

    strncpy(das->action, "05", 2);

    snprintf(das->callid, 32, "%lu", svm->callid);
    snprintf(das->capture_time, 16, "%lu", g_time);
    snprintf(das->starttime, 16, "%lu", svm->starttime);
    snprintf(das->endtime, 16, "%lu", svm->endtime);
    snprintf(das->lasttime, 16, "%d", svm->duration);
    if (svm->fromphone[0] != '-') {
        snprintf(das->calling_num, 32, "%s", svm->fromphone);
    }
    if (svm->tophone[0] != '-') {
        snprintf(das->called_num, 32, "%s", svm->tophone);
    }

    snprintf(das->linklayerid, 16, "%d", svm->linklayerid);
    snprintf(das->inunit, 32, "%s", svm->inunit);
    snprintf(das->outunit, 32, "%s", svm->outunit);

    snprintf(das->path, 16, "%d", svm->path);
    snprintf(das->callflag, 16, "%d", svm->callflag);
    //if (svm->userid != 0) {
        snprintf(das->cbid1, 16, "%u", svm->userid);
        snprintf(das->voice_id1, 16, "%u", svm->voiceid);
        snprintf(das->score1, 16, "%u", svm->percent);
    //}

    strncpy(das->online_flag, "0", 1);

    if (svm->callflag == 1) {
        snprintf(das->object_id, 16, "%u", CALL_FAX);
    } else {
        snprintf(das->object_id, 16, "%u", CALL_VOICE);
    }

    return 0;
}

int svm2das_case_voice(voice_record_t *svm, das_record_t *das, unsigned int object_id)
{
    unsigned int id;
    memset(das, 0, sizeof(das_record_t));

    id = atomic32_add_return(&das_session, 1);
    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);
    strncpy(das->systype, "1", 1);

    strncpy(das->data_source, "111", 3);
    strncpy(das->isp_id, "01", 2);

    strncpy(das->action, "05", 2);

    snprintf(das->callid, 32, "%lu", svm->callid);
    snprintf(das->clue_id, 16, "%u", svm->ruleid);
    snprintf(das->capture_time, 16, "%lu", g_time);
    snprintf(das->starttime, 16, "%lu", svm->starttime);
    snprintf(das->endtime, 16, "%lu", svm->endtime);
    snprintf(das->lasttime, 16, "%d", svm->duration);
    if (svm->fromphone[0] != '-') {
        snprintf(das->calling_num, 32, "%s", svm->fromphone);
    }
    if (svm->tophone[0] != '-') {
        snprintf(das->called_num, 32, "%s", svm->tophone);
    }

    snprintf(das->linklayerid, 16, "%d", svm->linklayerid);
    snprintf(das->inunit, 32, "%s", svm->inunit);
    snprintf(das->outunit, 32, "%s", svm->outunit);

    snprintf(das->path, 16, "%d", svm->path);
    snprintf(das->callflag, 16, "%d", svm->callflag);
    //if (svm->userid != 0) {
        snprintf(das->cbid1, 16, "%u", svm->userid);
        snprintf(das->voice_id1, 16, "%u", svm->voiceid);
        snprintf(das->score1, 16, "%u", svm->percent);
    //}
    /*将语音的方向存入到数据库中，暂时存入到备用字段C2中*/
    snprintf(das->c2, 255, "%u", svm->vrsflag); /** svm->vrsflag暂时存放了语音的方向，1代表上行，2代表下行 */
    snprintf(das->object_id, 16, "%u", object_id);
    strncpy(das->online_flag, "0", 1);

    return 0;
}

int svm2das_mass_sms(sms_record_t *svm, das_record_t *das)
{
    unsigned int id;
    memset(das, 0, sizeof(das_record_t));

    id = atomic32_add_return(&das_session, 1);
    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);
    strncpy(das->systype, "1", 1);

    strncpy(das->data_source, "111", 3);
    strncpy(das->isp_id, "01", 2);
    if (strcmp(svm->action, "03") == 0) {
        strncpy(das->action, "07", 2);
    } else {
        strncpy(das->action, "08", 2);
    }

    snprintf(das->callid, 32, "%lu", svm->smsid);

    snprintf(das->capture_time, 16, "%lu", g_time);

    snprintf(das->starttime, 16, "%lu", svm->sendtime);
    snprintf(das->endtime, 16, "%lu", svm->recvtime);
    snprintf(das->lasttime, 16, "%d", 0);

    if (svm->fromphone[0] != '-') {
        snprintf(das->calling_num, 32, "%s", svm->fromphone);
    }
    if (svm->tophone[0] != '-') {
        snprintf(das->called_num, 32, "%s", svm->tophone);
    }
    if (svm->fromimsi[0] != '-') {
        snprintf(das->imsi, 32, "%s", svm->fromimsi);
    }
    if (svm->toimsi[0] != '-') {
        snprintf(das->bimsi, 32, "%s", svm->toimsi);
    }

    snprintf(das->smstext, 2048, "%s", svm->content);

    strncpy(das->online_flag, "0", 1);

    snprintf(das->cbid1, 16, "%u", 0);
    snprintf(das->voice_id1, 16, "%u", 0);
    snprintf(das->score1, 16, "%u", 0);

    snprintf(das->callflag, 16, "%d", 0);

    snprintf(das->object_id, 16, "%u", CALL_SMS);

    return 0;
}


int svm2das_case_sms(sms_record_t *svm, das_record_t *das, unsigned int object_id)
{
    unsigned int id;
    memset(das, 0, sizeof(das_record_t));

    id = atomic32_add_return(&das_session, 1);
    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);
    strncpy(das->systype, "1", 1);
    strncpy(das->data_source, "111", 3);
    strncpy(das->isp_id, "01", 2);

    if (strcmp(svm->action, "03") == 0) {
        strncpy(das->action, "07", 2);
    } else {
        strncpy(das->action, "08", 2);
    }

    snprintf(das->callid, 32, "%lu", svm->smsid);
    snprintf(das->clue_id, 16, "%u", svm->ruleid);

    snprintf(das->capture_time, 16, "%lu", g_time);
    snprintf(das->starttime, 16, "%lu", svm->sendtime);
    snprintf(das->endtime, 16, "%lu", svm->recvtime);
    snprintf(das->lasttime, 16, "%d", 0);

    if (svm->fromphone[0] != '-') {
        snprintf(das->calling_num, 32, "%s", svm->fromphone);
    }
    if (svm->tophone[0] != '-') {
        snprintf(das->called_num, 32, "%s", svm->tophone);
    }
    if (svm->fromimsi[0] != '-') {
        snprintf(das->imsi, 32, "%s", svm->fromimsi);
    }
    if (svm->toimsi[0] != '-') {
        snprintf(das->bimsi, 32, "%s", svm->toimsi);
    }

    snprintf(das->smstext, 2048, "%s", svm->content);

    snprintf(das->object_id, 16, "%u", object_id);
    strncpy(das->online_flag, "0", 1);

    snprintf(das->cbid1, 16, "%u", 0);
    snprintf(das->voice_id1, 16, "%u", 0);
    snprintf(das->score1, 16, "%u", 0);
    snprintf(das->callflag, 16, "%d", 0);

    return 0;
}





int mobile2das_mass(mobile_mass_t *mass, das_record_t *das)
{
    int acttype;
    struct tm tm_time;  
    int unixtime;
    unsigned int id;

    if (mass == NULL || das == NULL) {
        return -1;
    }

    memset(das, 0, sizeof(das_record_t));

    id = atomic32_add_return(&das_session, 1);

    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);
    strncpy(das->systype, "2", 1);
    acttype = atoi(mass->acttype);
    if (acttype == 0x302 || acttype == 0x304) {
        strncpy(das->called_num, mass->aisdn, 24);
        strncpy(das->calling_num, mass->bisdn, 24);
        //strncpy(das->calling_num, mass->bisdnx, 24);
    } else {
        strncpy(das->calling_num, mass->aisdn, 24);
        strncpy(das->called_num, mass->bisdn, 24);
        //strncpy(das->called_num, mass->bisdnx, 24);
    }

    //strncpy(das->data_source, "124", 3);
    strncpy(das->data_source, "111", 3);
    strncpy(das->isp_id, "01", 2);
    strncpy(das->ahome, mass->ahome, 24);
    strncpy(das->imsi, mass->aimsi, 16);
    strncpy(das->imei, mass->aimei, 16);
    strncpy(das->aneid, mass->aneid, 24);
    strncpy(das->amcc, mass->amcc, 16);
    strncpy(das->amnc, mass->amnc, 16);
    strncpy(das->alacs, mass->alacs, 16);
    strncpy(das->acells, mass->acells, 16);
    strncpy(das->alace, mass->alace, 16);
    strncpy(das->acelle, mass->acelle, 16);
    strncpy(das->apc, mass->apc, 16);

    //strncpy(das->bisdnx, mass->bisdnx, 24);
    strncpy(das->bisdnx, mass->bisdn, 24);
    strncpy(das->bhome, mass->bhome, 24);
    strncpy(das->btisdn, mass->btisdn, 48);
    strncpy(das->bimsi, mass->bimsi, 24);
    strncpy(das->bneid, mass->bneid, 24);
    strncpy(das->bpc, mass->bpc, 24);
    switch(acttype) {
        case 0x103:
            strncpy(das->action, "01", 2);
            break;
        case 0x104:
            strncpy(das->action, "02", 2);
            break;
        case 0x101:
            strncpy(das->action, "03", 2);
            break;
        case 0x102:
            strncpy(das->action, "04", 2);
            break;
        case 0x301:
            strncpy(das->action, "05", 2);
            break;
        case 0x302:
            strncpy(das->action, "06", 2);
            break;
        case 0x303:
            strncpy(das->action, "07", 2);
            break;
        case 0x304:
            strncpy(das->action, "08", 2);
            break;
        case 0x305:
            strncpy(das->action, "09", 2);
            break;
        case 0x306:
            strncpy(das->action, "10", 2);
            break;
        case 0x400:
            strncpy(das->action, "11", 2);
            break;
    }

    snprintf(das->capture_time, 16, "%lu", g_time);
    if (0 != strptime(mass->starttime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->starttime, 16, "%u", unixtime);
    } else {
        snprintf(das->starttime, 16, "%u", 0);
    }

    if (0 != strptime(mass->endtime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->endtime, 16, "%u", unixtime);
    } else {
        snprintf(das->endtime, 16, "%u", 0);
    }
    if (acttype == 0x301 || acttype == 0x302) {
        strncpy(das->lasttime, mass->duration, 16);
    } else {
        strncpy(das->lasttime, "0", 16);
    }
    strncpy(das->vocfile, mass->vocfile, 64);
    if (strlen(mass->vocfile) > 0) {
        snprintf(das->path, 16, "1");
    } else {
        snprintf(das->path, 16, "0");
    }
    strncpy(das->extinfo, mass->extinfo, 64);
    strncpy(das->result, mass->result, 64);
    strncpy(das->cause, mass->cause, 64);
    //strncpy(das->, mass->netid, 64);
    strncpy(das->smslan, mass->smslan, 64);
    strncpy(das->smstext, mass->smstext, 2048);

    snprintf(das->callid, 16, "%u", id);
    strncpy(das->callflag, "0", 1);
    strncpy(das->online_flag, "0", 1);

    snprintf(das->cbid1, 16, "%u", 0);
    snprintf(das->voice_id1, 16, "%u", 0);
    snprintf(das->score1, 16, "%u", 0);

    if (acttype == 0x301 || acttype == 0x302) {
        snprintf(das->object_id, 16, "%u", CALL_VOICE);
    } else if (acttype == 0x303 || acttype == 0x304) {
        snprintf(das->object_id, 16, "%u", CALL_SMS);
    } else {
        snprintf(das->object_id, 16, "%u", CALL_OTHER);
    }

    return 0;
}


int mobile2das_case(mobile_case_t *cases, das_record_t *das, unsigned int object_id)
{
    int acttype;
    struct tm tm_time;  
    int unixtime;
    uint32_t id;
    uint32_t clue_id;

    memset(das, 0, sizeof(das_record_t));

    clue_id = atoi(cases->mntid) - 200;
    snprintf(das->clue_id, 16, "%u", clue_id);

    id = atomic32_add_return(&das_session, 1);

    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);
    strncpy(das->systype, "2", 1);

    acttype = atoi(cases->acttype);
    if (acttype == 0x302 || acttype == 0x304) {
        strncpy(das->called_num, cases->aisdn, 24);
        strncpy(das->calling_num, cases->bisdn, 24);
        //strncpy(das->calling_num, cases->bisdnx, 24);
    } else {
        strncpy(das->calling_num, cases->aisdn, 24);
        strncpy(das->called_num, cases->bisdn, 24);
        //strncpy(das->called_num, cases->bisdnx, 24);
    }

    //strncpy(das->data_source, "124", 3);
    strncpy(das->data_source, "111", 3);
    strncpy(das->isp_id, "01", 2);
    strncpy(das->ahome, cases->ahome, 24);
    strncpy(das->imsi, cases->aimsi, 16);
    strncpy(das->imei, cases->aimei, 16);
    strncpy(das->aneid, cases->aneid, 24);
    strncpy(das->amcc, cases->amcc, 16);
    strncpy(das->amnc, cases->amnc, 16);
    strncpy(das->alacs, cases->alacs, 16);
    strncpy(das->acells, cases->acells, 16);
    strncpy(das->alace, cases->alace, 16);
    strncpy(das->acelle, cases->acelle, 16);
    strncpy(das->apc, cases->apc, 16);

    //strncpy(das->bisdnx, cases->bisdnx, 24);
    strncpy(das->bisdnx, cases->bisdn, 24);
    strncpy(das->bhome, cases->bhome, 24);
    strncpy(das->btisdn, cases->btisdn, 48);
    strncpy(das->bimsi, cases->bimsi, 24);
    strncpy(das->bneid, cases->bneid, 24);
    strncpy(das->bpc, cases->bpc, 24);
    switch(acttype) {
        case 0x103:
            strncpy(das->action, "01", 2);
            break;
        case 0x104:
            strncpy(das->action, "02", 2);
            break;
        case 0x101:
            strncpy(das->action, "03", 2);
            break;
        case 0x102:
            strncpy(das->action, "04", 2);
            break;
        case 0x301:
            strncpy(das->action, "05", 2);
            break;
        case 0x302:
            strncpy(das->action, "06", 2);
            break;
        case 0x303:
            strncpy(das->action, "07", 2);
            break;
        case 0x304:
            strncpy(das->action, "08", 2);
            break;
        case 0x305:
            strncpy(das->action, "09", 2);
            break;
        case 0x306:
            strncpy(das->action, "10", 2);
            break;
        case 0x400:
            strncpy(das->action, "11", 2);
            break;
    }
    snprintf(das->capture_time, 16, "%lu", g_time);
    if (0 != strptime(cases->starttime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->starttime, 16, "%u", unixtime);
    } else {
        snprintf(das->starttime, 16, "%u", 0);
    }

    if (0 != strptime(cases->endtime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->endtime, 16, "%u", unixtime);
    } else {
        snprintf(das->endtime, 16, "%u", 0);
    }

    if (acttype == 0x301 || acttype == 0x302) {
        strncpy(das->lasttime, cases->duration, 16);
    } else {
        strncpy(das->lasttime, "0", 16);
    }
    strncpy(das->vocfile, cases->vocfile, 64);
    if (strlen(cases->vocfile) > 0) {
        snprintf(das->path, 16, "1");
    } else {
        snprintf(das->path, 16, "0");
    }
    strncpy(das->extinfo, cases->extinfo, 64);
    strncpy(das->result, cases->result, 64);
    strncpy(das->cause, cases->cause, 64);
    //strncpy(das->, cases->netid, 64);
    strncpy(das->smslan, cases->smslan, 64);
    strncpy(das->smstext, cases->smstext, 2048);

    snprintf(das->callid, 16, "%u", id);
    strncpy(das->callflag, "0", 1);
    strncpy(das->online_flag, "0", 1);
    snprintf(das->object_id, 16, "%u", object_id);

    snprintf(das->cbid1, 16, "%u", 0);
    snprintf(das->voice_id1, 16, "%u", 0);
    snprintf(das->score1, 16, "%u", 0);

    return 0;
}




#if 0
int voipa2das_mass(voipa_mass_t *mass, voip_record_t *das)
{
    int acttype;
    struct tm tm_time;  
    int unixtime;
    unsigned int id;

    if (mass == NULL || das == NULL) {
        return -1;
    }

    memset(das, 0, sizeof(voip_record_t));

    id = atomic32_add_return(&das_session, 1);

    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);

    snprintf(das->capture_time, 16, "%lu", g_time);
    snprintf(das->voip_type, 12, "%u", 1);

    strncpy(das->isp_id, "01", 2);
    strncpy(das->recordflag, "0", 1);

    acttype = atoi(mass->acttype);
    if (acttype == 0x302 || acttype == 0x304) {
        strncpy(das->to_id, mass->aisdn, 24);
        //strncpy(das->from_id, mass->bisdn, 24);
        strncpy(das->from_id, mass->bisdnx, 24);
    } else {
        strncpy(das->from_id, mass->aisdn, 24);
        //strncpy(das->to_id, mass->bisdn, 24);
        strncpy(das->to_id, mass->bisdnx, 24);
    }

    //strncpy(das->data_source, "124", 3);
    strncpy(das->data_source, "111", 3);

    strncpy(das->imsi, mass->aimsi, 16);

    switch(acttype) {
        case 0x103://power on
            strncpy(das->action, "01", 2);
            break;
        case 0x104://power off
            strncpy(das->action, "02", 2);
            break;
        case 0x101://location update
            strncpy(das->action, "03", 2);
            break;
        case 0x102://periodic update
            strncpy(das->action, "04", 2);
            break;
        case 0x301://calling
            strncpy(das->action, "05", 2);
            break;
        case 0x302://called
            strncpy(das->action, "06", 2);
            break;
        case 0x303://send sms
            strncpy(das->action, "07", 2);
            break;
        case 0x304://receive sms
            strncpy(das->action, "08", 2);
            break;
        case 0x305://handover
            strncpy(das->action, "09", 2);
            break;
        case 0x306://paging
            strncpy(das->action, "10", 2);
            break;
        case 0x400://supplementary Service
            strncpy(das->action, "11", 2);
            break;
    }

    if (0 != strptime(mass->starttime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        //snprintf(das->starttime, 16, "%u", unixtime);
    } else {
        //snprintf(das->starttime, 16, "%u", 0);
    }

    if (0 != strptime(mass->endtime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        //snprintf(das->endtime, 16, "%u", unixtime);
    } else {
        //snprintf(das->endtime, 16, "%u", 0);
    }
    strncpy(das->content, mass->smstext, 2048);

    return 0;
}
#else
int voipa2das_mass(voipa_mass_t *mass, voip_record_t *das)
{
    int acttype;
    struct tm tm_time;  
    unsigned int starttime = 0;
    unsigned int endtime = 0;
    unsigned int lasttime = 0;
    unsigned int id;
    time_t g_time;

    if (mass == NULL || das == NULL) {
        return -1;
    }

    memset(das, 0, sizeof(voip_record_t));

    g_time = time(NULL);
    snprintf(das->capture_time, 16, "%lu", g_time);
    strcpy(das->data_source, "111");
    strcpy(das->company_name, "semptian");
    //strcpy(das->collect_place, "320100");
    strcpy(das->collect_place, cfu_conf.src_city_code);
    strcpy(das->app_layer_protocol, "99");
    id = atomic32_add_return(&das_session, 1);
    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->record_id, 16, "%u", id);
    strcpy(das->voip_type, "1099999");

    acttype = atoi(mass->acttype);
    if (acttype == 0x302 || acttype == 0x304) {
        COPY_ISDN(das->to_id, mass->aisdn, 24);
        COPY_ISDN(das->receiver_phone, mass->aisdn, 24);
        COPY_ISDN(das->from_id, mass->bisdn, 24);
        //strncpy(das->from_id, mass->bisdnx, 24);
        COPY_ISDN(das->sender_phone, mass->bisdn, 24);
        //strncpy(das->sender_phone, mass->bisdnx, 24);
    } else {
        COPY_ISDN(das->from_id, mass->aisdn, 24);
        COPY_ISDN(das->sender_phone, mass->aisdn, 24);
        COPY_ISDN(das->to_id, mass->bisdn, 24);
        //strncpy(das->to_id, mass->bisdnx, 24);
        COPY_ISDN(das->receiver_phone, mass->bisdn, 24);
        //strncpy(das->receiver_phone, mass->bisdnx, 24);
    }

    if (0 != strptime(mass->starttime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        starttime = mktime(&tm_time);
        snprintf(das->call_starttime, 16, "%u", starttime);
    } else {
        snprintf(das->call_starttime, 16, "%u", 0);
    }

    if (0 != strptime(mass->endtime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        endtime = mktime(&tm_time);
        lasttime = endtime - starttime;
        snprintf(das->call_usedtime, 16, "%u", lasttime);
    } else {
        snprintf(das->call_usedtime, 16, "%u", 0);
    }

    switch(acttype) {
        case 0x103://power on
            strncpy(das->action, "99", 2);
            break;
        case 0x104://power off
            strncpy(das->action, "99", 2);
            break;
        case 0x101://location update
            strncpy(das->action, "99", 2);
            break;
        case 0x102://periodic update
            strncpy(das->action, "99", 2);
            break;
        case 0x301://calling
            strncpy(das->action, "20", 2);
            break;
        case 0x302://called
            strncpy(das->action, "21", 2);
            break;
        case 0x303://send sms
            strncpy(das->action, "20", 2);
            break;
        case 0x304://receive sms
            strncpy(das->action, "21", 2);
            break;
        case 0x305://handover
            strncpy(das->action, "99", 2);
            break;
        case 0x306://paging
            strncpy(das->action, "99", 2);
            break;
        case 0x400://supplementary Service
            strncpy(das->action, "99", 2);
            break;
    }

    return 0;
}


#endif


int voipa2das_case(voipa_case_t *cases, das_record_t *das, unsigned int object_id)
{
    int acttype;
    struct tm tm_time;  
    int unixtime;
    uint32_t id;
    uint32_t clue_id;

    memset(das, 0, sizeof(das_record_t));

    clue_id = atoi(cases->mntid) - 200;
    snprintf(das->clue_id, 16, "%u", clue_id);
    //strncpy(das->clue_id, cases->mntid, 16);

    id = atomic32_add_return(&das_session, 1);

    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);
    strncpy(das->systype, "3", 1);

    acttype = atoi(cases->acttype);
    if (acttype == 0x302 || acttype == 0x304) {
        COPY_ISDN(das->called_num, cases->aisdn, 24);
        COPY_ISDN(das->calling_num, cases->bisdn, 24);
        //strncpy(das->calling_num, cases->bisdnx, 24);
    } else {
        COPY_ISDN(das->calling_num, cases->aisdn, 24);
        COPY_ISDN(das->called_num, cases->bisdn, 24);
        //strncpy(das->called_num, cases->bisdnx, 24);
    }

    //strncpy(das->data_source, "124", 3);
    strncpy(das->data_source, "111", 3);
    strncpy(das->isp_id, "01", 2);
    strncpy(das->ahome, cases->ahome, 24);
    strncpy(das->imsi, cases->aimsi, 16);
    strncpy(das->imei, cases->aimei, 16);
    strncpy(das->aneid, cases->aneid, 24);
    strncpy(das->amcc, cases->amcc, 16);
    strncpy(das->amnc, cases->amnc, 16);
    strncpy(das->alacs, cases->alacs, 16);
    strncpy(das->acells, cases->acells, 16);
    strncpy(das->alace, cases->alace, 16);
    strncpy(das->acelle, cases->acelle, 16);
    strncpy(das->apc, cases->apc, 16);

    //strncpy(das->bisdnx, cases->bisdnx, 24);
    COPY_ISDN(das->bisdnx, cases->bisdn, 24);
    strncpy(das->bhome, cases->bhome, 24);
    strncpy(das->btisdn, cases->btisdn, 48);
    strncpy(das->bimsi, cases->bimsi, 24);
    strncpy(das->bneid, cases->bneid, 24);
    strncpy(das->bpc, cases->bpc, 24);

	strncpy(das->upareaid, cfu_conf.src_city_code, 15);

    switch(acttype) {
        case 0x103:
            strncpy(das->action, "01", 2);
            break;
        case 0x104:
            strncpy(das->action, "02", 2);
            break;
        case 0x101:
            strncpy(das->action, "03", 2);
            break;
        case 0x102:
            strncpy(das->action, "04", 2);
            break;
        case 0x301:
            strncpy(das->action, "05", 2);
            break;
        case 0x302:
            strncpy(das->action, "06", 2);
            break;
        case 0x303:
            strncpy(das->action, "07", 2);
            break;
        case 0x304:
            strncpy(das->action, "08", 2);
            break;
        case 0x305:
            strncpy(das->action, "09", 2);
            break;
        case 0x306:
            strncpy(das->action, "10", 2);
            break;
        case 0x400:
            strncpy(das->action, "11", 2);
            break;
    }
    snprintf(das->capture_time, 16, "%lu", g_time);
    if (0 != strptime(cases->starttime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->starttime, 16, "%u", unixtime);
    } else {
        snprintf(das->starttime, 16, "%u", 0);
    }

    if (0 != strptime(cases->endtime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->endtime, 16, "%u", unixtime);
    } else {
        snprintf(das->endtime, 16, "%u", 0);
    }
    if (acttype == 0x301 || acttype == 0x302) {
        strncpy(das->lasttime, cases->duration, 16);
    } else {
        strncpy(das->lasttime, "0", 16);
    }
    strncpy(das->vocfile, cases->vocfile, 64);
    if (strlen(cases->vocfile) > 0) {
        snprintf(das->path, 16, "1");
    } else {
        snprintf(das->path, 16, "0");
    }
    strncpy(das->extinfo, cases->extinfo, 64);
    strncpy(das->result, cases->result, 64);
    strncpy(das->cause, cases->cause, 64);
    //strncpy(das->, cases->netid, 64);
    strncpy(das->smslan, cases->smslan, 64);
    strncpy(das->smstext, cases->smstext, 2048);

    snprintf(das->callid, 16, "%u", id);
    strncpy(das->callflag, "0", 1);
    strncpy(das->online_flag, "0", 1);
    snprintf(das->object_id, 16, "%u", object_id);

    snprintf(das->cbid1, 16, "%u", 0);
    snprintf(das->voice_id1, 16, "%u", 0);
    snprintf(das->score1, 16, "%u", 0);

    return 0;
}






int voipb2das_mass(voipb_mass_t *mass, das_record_t *das)
{
    int acttype;
    struct tm tm_time;  
    int unixtime;
    unsigned int id;

    if (mass == NULL || das == NULL) {
        return -1;
    }

    memset(das, 0, sizeof(das_record_t));

    id = atomic32_add_return(&das_session, 1);

    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);
    strncpy(das->systype, "4", 1);

    acttype = atoi(mass->acttype);
    if (acttype == 0x302 || acttype == 0x304) {
        COPY_ISDN(das->called_num, mass->aisdn, 24);
        COPY_ISDN(das->calling_num, mass->bisdn, 24);
        //strncpy(das->calling_num, mass->bisdnx, 24);
    } else {
        COPY_ISDN(das->calling_num, mass->aisdn, 24);
        COPY_ISDN(das->called_num, mass->bisdn, 24);
        //strncpy(das->called_num, mass->bisdnx, 24);
    }

    //strncpy(das->data_source, "124", 3);
    strncpy(das->data_source, "111", 3);
    strncpy(das->isp_id, "01", 2);
    strncpy(das->ahome, mass->ahome, 24);
    strncpy(das->imsi, mass->aimsi, 16);
    strncpy(das->imei, mass->aimei, 16);
    strncpy(das->aneid, mass->aneid, 24);
    strncpy(das->amcc, mass->amcc, 16);
    strncpy(das->amnc, mass->amnc, 16);
    strncpy(das->alacs, mass->alacs, 16);
    strncpy(das->acells, mass->acells, 16);
    strncpy(das->alace, mass->alace, 16);
    strncpy(das->acelle, mass->acelle, 16);
    strncpy(das->apc, mass->apc, 16);

    //strncpy(das->bisdnx, mass->bisdnx, 24);
    COPY_ISDN(das->bisdnx, mass->bisdn, 24);
    strncpy(das->bhome, mass->bhome, 24);
    strncpy(das->btisdn, mass->btisdn, 48);
    strncpy(das->bimsi, mass->bimsi, 24);
    strncpy(das->bneid, mass->bneid, 24);
    strncpy(das->bpc, mass->bpc, 24);

    switch(acttype) {
        case 0x103:
            strncpy(das->action, "01", 2);
            break;
        case 0x104:
            strncpy(das->action, "02", 2);
            break;
        case 0x101:
            strncpy(das->action, "03", 2);
            break;
        case 0x102:
            strncpy(das->action, "04", 2);
            break;
        case 0x301:
            strncpy(das->action, "05", 2);
            break;
        case 0x302:
            strncpy(das->action, "06", 2);
            break;
        case 0x303:
            strncpy(das->action, "07", 2);
            break;
        case 0x304:
            strncpy(das->action, "08", 2);
            break;
        case 0x305:
            strncpy(das->action, "09", 2);
            break;
        case 0x306:
            strncpy(das->action, "10", 2);
            break;
        case 0x400:
            strncpy(das->action, "11", 2);
            break;
    }

    snprintf(das->capture_time, 16, "%lu", g_time);
    if (0 != strptime(mass->starttime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->starttime, 16, "%u", unixtime);
    } else {
        snprintf(das->starttime, 16, "%u", 0);
    }

    if (0 != strptime(mass->endtime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->endtime, 16, "%u", unixtime);
    } else {
        snprintf(das->endtime, 16, "%u", 0);
    }
    if (acttype == 0x301 || acttype == 0x302) {
        strncpy(das->lasttime, mass->duration, 16);
    } else {
        strncpy(das->lasttime, "0", 16);
    }
    strncpy(das->vocfile, mass->vocfile, 64);
    if (strlen(mass->vocfile) > 0) {
        snprintf(das->path, 16, "1");
    } else {
        snprintf(das->path, 16, "0");
    }
    strncpy(das->extinfo, mass->extinfo, 64);
    strncpy(das->result, mass->result, 64);
    strncpy(das->cause, mass->cause, 64);
    //strncpy(das->, mass->netid, 64);
    strncpy(das->smslan, mass->smslan, 64);
    strncpy(das->smstext, mass->smstext, 2048);

    snprintf(das->callid, 16, "%u", id);
    strncpy(das->callflag, "0", 1);
    strncpy(das->online_flag, "0", 1);

    snprintf(das->cbid1, 16, "%u", 0);
    snprintf(das->voice_id1, 16, "%u", 0);
    snprintf(das->score1, 16, "%u", 0);

    if (acttype == 0x301 || acttype == 0x302) {
        snprintf(das->object_id, 16, "%u", CALL_VOICE);
    } else if (acttype == 0x303 || acttype == 0x304) {
        snprintf(das->object_id, 16, "%u", CALL_SMS);
    } else {
        snprintf(das->object_id, 16, "%u", CALL_OTHER);
    }

    return 0;
}


int voipb2das_case(voipb_case_t *cases, das_record_t *das, unsigned int object_id)
{
    int acttype;
    struct tm tm_time;  
    int unixtime;
    uint32_t id;
    uint32_t clue_id;

    memset(das, 0, sizeof(das_record_t));

    clue_id = atoi(cases->mntid) - 200;
    snprintf(das->clue_id, 16, "%u", clue_id);
    //strncpy(das->clue_id, cases->mntid, 16);

    id = atomic32_add_return(&das_session, 1);

    id = (g_time << 16) | (id & 0xffff);
    snprintf(das->id, 16, "%u", id);
    strncpy(das->systype, "4", 1);

    acttype = atoi(cases->acttype);
    if (acttype == 0x302 || acttype == 0x304) {
        COPY_ISDN(das->called_num, cases->aisdn, 24);
        COPY_ISDN(das->calling_num, cases->bisdn, 24);
        //strncpy(das->calling_num, cases->bisdnx, 24);
    } else {
        COPY_ISDN(das->calling_num, cases->aisdn, 24);
        COPY_ISDN(das->called_num, cases->bisdn, 24);
        //strncpy(das->called_num, cases->bisdnx, 24);
    }

    //strncpy(das->data_source, "124", 3);
    strncpy(das->data_source, "111", 3);
    strncpy(das->isp_id, "01", 2);
    strncpy(das->ahome, cases->ahome, 24);
    strncpy(das->imsi, cases->aimsi, 16);
    strncpy(das->imei, cases->aimei, 16);
    strncpy(das->aneid, cases->aneid, 24);
    strncpy(das->amcc, cases->amcc, 16);
    strncpy(das->amnc, cases->amnc, 16);
    strncpy(das->alacs, cases->alacs, 16);
    strncpy(das->acells, cases->acells, 16);
    strncpy(das->alace, cases->alace, 16);
    strncpy(das->acelle, cases->acelle, 16);
    strncpy(das->apc, cases->apc, 16);

    COPY_ISDN(das->bisdnx, cases->bisdn, 24);
    strncpy(das->bhome, cases->bhome, 24);
    strncpy(das->btisdn, cases->btisdn, 48);
    strncpy(das->bimsi, cases->bimsi, 24);
    strncpy(das->bneid, cases->bneid, 24);
    strncpy(das->bpc, cases->bpc, 24);

    switch(acttype) {
        case 0x103:
            strncpy(das->action, "01", 2);
            break;
        case 0x104:
            strncpy(das->action, "02", 2);
            break;
        case 0x101:
            strncpy(das->action, "03", 2);
            break;
        case 0x102:
            strncpy(das->action, "04", 2);
            break;
        case 0x301:
            strncpy(das->action, "05", 2);
            break;
        case 0x302:
            strncpy(das->action, "06", 2);
            break;
        case 0x303:
            strncpy(das->action, "07", 2);
            break;
        case 0x304:
            strncpy(das->action, "08", 2);
            break;
        case 0x305:
            strncpy(das->action, "09", 2);
            break;
        case 0x306:
            strncpy(das->action, "10", 2);
            break;
        case 0x400:
            strncpy(das->action, "11", 2);
            break;
    }
    snprintf(das->capture_time, 16, "%lu", g_time);
    if (0 != strptime(cases->starttime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->starttime, 16, "%u", unixtime);
    } else {
        snprintf(das->starttime, 16, "%u", 0);
    }

    if (0 != strptime(cases->endtime, "%Y-%m-%d %H:%M:%S", &tm_time)) {
        unixtime = mktime(&tm_time);
        snprintf(das->endtime, 16, "%u", unixtime);
    } else {
        snprintf(das->endtime, 16, "%u", 0);
    }
    if (acttype == 0x301 || acttype == 0x302) {
        strncpy(das->lasttime, cases->duration, 16);
    } else {
        strncpy(das->lasttime, "0", 16);
    }
    strncpy(das->vocfile, cases->vocfile, 64);
    if (strlen(cases->vocfile) > 0) {
        snprintf(das->path, 16, "1");
    } else {
        snprintf(das->path, 16, "0");
    }
    strncpy(das->extinfo, cases->extinfo, 64);
    strncpy(das->result, cases->result, 64);
    strncpy(das->cause, cases->cause, 64);
    //strncpy(das->, cases->netid, 64);
    strncpy(das->smslan, cases->smslan, 64);
    strncpy(das->smstext, cases->smstext, 2048);

    snprintf(das->callid, 16, "%u", id);
    strncpy(das->callflag, "0", 1);
    strncpy(das->online_flag, "0", 1);
    snprintf(das->object_id, 16, "%u", object_id);

    snprintf(das->cbid1, 16, "%u", 0);
    snprintf(das->voice_id1, 16, "%u", 0);
    snprintf(das->score1, 16, "%u", 0);

    return 0;
}




int write_bcp(FILE *fp, das_record_t *das_record)
{
    if (fp == NULL) {
        return -1;
    }

    
    fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t0\n", das_record->id, das_record->clue_id, das_record->base_station_id, das_record->location_id, das_record->ci, das_record->station_type, das_record->imsi, das_record->bhome, das_record->imei, das_record->apc, das_record->calling_num, das_record->called_num, das_record->action, das_record->call_type, das_record->upareaid, das_record->data_source, das_record->capture_time, das_record->starttime, das_record->endtime, das_record->lasttime, das_record->isp_id, das_record->local_bsattribution, das_record->nonlocal_bsattribution, das_record->phoneattribution, das_record->n_phoneattribution, das_record->tmsi, das_record->longitude, das_record->latitude, das_record->alacs, das_record->acells, das_record->alace, das_record->acelle, das_record->btisdn, das_record->vocfile, das_record->extinfo, das_record->result, das_record->cause, das_record->ispid, das_record->mntid, das_record->smslan, das_record->smstext, das_record->callid, das_record->callflag, das_record->cbid1, das_record->cbid2, das_record->cbid3, das_record->score1, das_record->score2, das_record->score3, das_record->voice_id1, das_record->voice_id2, das_record->voice_id3, das_record->remarkstr, das_record->c1, das_record->c2, das_record->i1, das_record->i2, das_record->object_id, das_record->ahome, das_record->aneid, das_record->amcc, das_record->amnc, das_record->bisdnx, das_record->bimsi, das_record->bneid, das_record->bpc, das_record->linklayerid, das_record->inunit, das_record->outunit, das_record->path, das_record->systype, das_record->online_flag, das_record->callcode);

    return 0;
}



#if 0
int write_voip_bcp(FILE *fp, voip_record_t *voip_record)
{
    if (fp == NULL) {
        return -1;
    }
    
    fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", 
            voip_record->userid, 
            voip_record->usertype, 
            voip_record->location,
            voip_record->application, 
            voip_record->appid, 
            voip_record->nbdomain, 
            voip_record->behavior, 
            voip_record->authtype, 
            voip_record->authaccount, 
            voip_record->period, 
            voip_record->nbdatasource, 
            voip_record->time, 
            voip_record->nbkeyword, 
            voip_record->nbkeywordid, 
            voip_record->terminalid, 
            voip_record->terminalname, 
            voip_record->tgdid, 
            voip_record->nbstore, 
            voip_record->trashruleid, 
            voip_record->objectid, 
            voip_record->id, 
            voip_record->clue_id, 
            voip_record->upareaid, 
            voip_record->clue_src_sys, 
            voip_record->clue_dst_sys, 
            voip_record->isp_id, 
            voip_record->device_id, 
            voip_record->lineid, 
            voip_record->src_ip, 
            voip_record->dst_ip, 
            voip_record->src_ipid, 
            voip_record->dst_ipid, 
            voip_record->strsrc_ip, 
            voip_record->strdst_ip, 
            voip_record->bsattribution, 
            voip_record->phoneattribution, 
            voip_record->teid, 
            voip_record->src_port, 
            voip_record->dst_port, 
            voip_record->mac, 
            voip_record->capture_time, 
            voip_record->data_source, 
            voip_record->company_name, 
            voip_record->country_type, 
            voip_record->certificate_code, 
            voip_record->certificate_type, 
            voip_record->sessionid, 
            voip_record->auth_type, 
            voip_record->relatedirection, 
            voip_record->auth_account, 
            voip_record->imsi, 
            voip_record->equipment_id, 
            voip_record->hardware_signature, 
            voip_record->servicecode, 
            voip_record->base_station_id, 
            voip_record->context, 
            voip_record->longitude, 
            voip_record->latitude, 
            voip_record->user_agent, 
            voip_record->tooltype, 
            voip_record->toolname, 
            voip_record->voip_type, 
            voip_record->chatroom, 
            voip_record->username, 
            voip_record->password, 
            voip_record->nickname, 
            voip_record->from_id, 
            voip_record->to_id, 
            voip_record->action, 
            voip_record->content, 
            voip_record->mainfile, 
            voip_record->filesize, 
            voip_record->audiofile1, 
            voip_record->audiofile2, 
            voip_record->videofile1, 
            voip_record->videofile2, 
            voip_record->itemflag, 
            voip_record->recordflag, 
            voip_record->superclass, 
            voip_record->subclass, 
            voip_record->remarkstr, 
            voip_record->c1, 
            voip_record->c2, 
            voip_record->i1, 
            voip_record->i2, 
            voip_record->roomid, 
            voip_record->src_auth_account, 
            voip_record->src_certificate_code, 
            voip_record->datalevel, 
            voip_record->activeareaid, 
            voip_record->vlanid, 
            voip_record->four_station_id, 
            voip_record->ip_type, 
            voip_record->src_ipv6, 
            voip_record->dst_ipv6, 
            voip_record->strsrc_ipv6, 
            voip_record->strdst_ipv6, 
            voip_record->src_ipidv6, 
            voip_record->dst_ipidv6, 
            voip_record->bitaction);

    return 0;
}
#else


int write_voip_bcp(FILE *fp, voip_record_t *voip_record)
{
    if (fp == NULL) {
        return -1;
    }
    
	
	
    fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", 
            voip_record->capture_time, 
            voip_record->data_source, 
            voip_record->company_name, 
            voip_record->collect_place, 
            voip_record->app_layer_protocol, 
            voip_record->record_id, 
            voip_record->voip_type, 
            voip_record->from_id, 
            voip_record->to_id, 
            voip_record->action, 
            voip_record->sender_phone, 
            voip_record->receiver_phone, 
            voip_record->call_starttime, 
            voip_record->call_usedtime);

    return 0;
}


#endif

