#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "conf_parser.h"
#include "../../common/applog.h"
#include "../../common/conf.h"
#include "../../common/thread_flag.h"
#include "../../common/iniparser.h"

#include "voipa_cdr_filter.h"

static st_voipa_fltr_rules_t * voipa_filter_rules = NULL;

#define GET_INI_STR(group, key, index, ini) (\
{\
    const char *str = NULL;\
    char keybuf[32] = {0};\
    snprintf(keybuf, 31, "%s:%s%d", group, key, index);\
    str = iniparser_getstring(ini, keybuf, NULL);\
})

#define FILL_ISDN_RULES(list, key) (\
{\
    for(i = 0; ; i++)\
    {\
        s = GET_INI_STR("isdn", key, i, ini);\
        if(NULL == s)\
        {\
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "read voipa filter isdn %s rule done, rules count: %d", key, i);\
            break;\
        }\
\
        st_voipa_fltr_isdn_rule_t * isdn_rule = (st_voipa_fltr_isdn_rule_t *)malloc(sizeof(st_voipa_fltr_isdn_rule_t));\
        if(NULL == isdn_rule)\
        {\
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "[%s:%d] fail to malloc(), size: %lu", __FILE__, __LINE__, sizeof(st_voipa_fltr_isdn_rule_t));\
            goto init_voipa_cdr_filter_ERROR_LIST;\
        }\
\
        snprintf(isdn_rule->reg_exp, 127, "%s", s);\
        if(regcomp(&isdn_rule->reg, isdn_rule->reg_exp, REG_EXTENDED) != 0)\
        {\
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "fail to regcomp(), check the pattern: %s", isdn_rule->reg_exp);\
            free(isdn_rule);\
            continue;\
        }\
\
        hlist_insert_head(list, &isdn_rule->node);\
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "add voipa filter isdn %s rule, reg_exp : %s", key, isdn_rule->reg_exp);\
    }\
})

#define DESTORY_RULE_LIST(list, type) (\
{\
    hlist_node_t * tmp = NULL;\
\
    if(list)\
    {\
        while((tmp = hlist_get_head_node(list)) != NULL)\
        {\
            type * rule = container_of(tmp, type, node);\
            free(rule);\
            rule = NULL;\
        }\
\
        free(list);\
        list = NULL;\
    }\
})

int voipa_iniparser_err_callback(const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, format, argptr);
    va_end(argptr);
    return 0;
}

/*
    @brief: 初始化voipa 话单过滤模块 
*/
int init_voipa_cdr_filter(void)
{
    int i = 0;
    const char *s;
    iniparser_set_error_callback(voipa_iniparser_err_callback);
    
    dictionary *ini = NULL;

    ini = iniparser_load(cfu_conf.voip_white_list_path);
    if(NULL == ini)
    {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "load voipa_cdr_filter_rule failed, path: %s", cfu_conf.voip_white_list_path);
        return -1;
    }

    voipa_filter_rules = (st_voipa_fltr_rules_t *)malloc(sizeof(st_voipa_fltr_rules_t));
    if(NULL == voipa_filter_rules)
    {
        applog( APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "[%s:%d] fail to malloc(), size: %lu", __FILE__, __LINE__, sizeof(st_voipa_fltr_rules_t));
        goto init_voipa_cdr_filter_ERROR_MALLOC;
    }

    voipa_filter_rules->acttype_rules = hlist_create();
    voipa_filter_rules->isdn_any_rules = hlist_create();
    voipa_filter_rules->isdn_both_rules = hlist_create();
    voipa_filter_rules->isdn_from_rules = hlist_create();
    voipa_filter_rules->isdn_to_rules = hlist_create();
    if(NULL == voipa_filter_rules->acttype_rules ||
        NULL == voipa_filter_rules->isdn_any_rules ||
        NULL == voipa_filter_rules->isdn_both_rules ||
        NULL == voipa_filter_rules->isdn_from_rules ||
        NULL == voipa_filter_rules->isdn_to_rules)
    {
        applog( APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "[%s:%d] fail to hlist_create()", __FILE__, __LINE__);
        goto init_voipa_cdr_filter_ERROR_LIST;
    }
    
    //读acttype规则
    for(i = 0; ; i++)
    {
        s = GET_INI_STR("acttype", "type_", i, ini);
        if(NULL == s)
        {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "read voipa filter acttype rule done, rules count: %d", i);
            break;
        }

        st_voipa_fltr_acttype_rule_t * filter_rule = (st_voipa_fltr_acttype_rule_t *)malloc(sizeof(st_voipa_fltr_acttype_rule_t));
        if(NULL == filter_rule)
        {
            applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, "[%s:%d] fail to malloc(), size: %lu", __FILE__, __LINE__, sizeof(st_voipa_fltr_acttype_rule_t));
            goto init_voipa_cdr_filter_ERROR_LIST;
        }
        
        filter_rule->acttype = atoi(s);

        hlist_insert_head(voipa_filter_rules->acttype_rules, &filter_rule->node);
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "add voipa filter acttype rule, acttype : %d", filter_rule->acttype);
    }

    FILL_ISDN_RULES(voipa_filter_rules->isdn_any_rules, "any_");
    FILL_ISDN_RULES(voipa_filter_rules->isdn_both_rules, "both_");
    FILL_ISDN_RULES(voipa_filter_rules->isdn_from_rules, "from_");
    FILL_ISDN_RULES(voipa_filter_rules->isdn_to_rules, "to_");

    iniparser_freedict(ini);
    
    return 0;

init_voipa_cdr_filter_ERROR_LIST:
    destory_voipa_cdr_filter();

init_voipa_cdr_filter_ERROR_MALLOC:
    iniparser_freedict(ini);
    
    return -1;
}

void destory_voipa_cdr_filter(void)
{
    if(!voipa_filter_rules)
    {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "no need to destory voipa cdr filter");
        return;
    }
        
    DESTORY_RULE_LIST(voipa_filter_rules->acttype_rules, st_voipa_fltr_acttype_rule_t);
    DESTORY_RULE_LIST(voipa_filter_rules->isdn_any_rules, st_voipa_fltr_isdn_rule_t);
    DESTORY_RULE_LIST(voipa_filter_rules->isdn_both_rules, st_voipa_fltr_isdn_rule_t);
    DESTORY_RULE_LIST(voipa_filter_rules->isdn_from_rules, st_voipa_fltr_isdn_rule_t);
    DESTORY_RULE_LIST(voipa_filter_rules->isdn_to_rules, st_voipa_fltr_isdn_rule_t);
    free(voipa_filter_rules);
    voipa_filter_rules = NULL;
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "destory voipa cdr filter sucess");
    return;
}

/*
    @brief: voipa 话单过滤业务
    @param: st_voipa_cdr_t *cdr         保存了cvs话单的内容
            enum_voipa_cdr_type_t type  表示是海量还是案件
    @return:    0 丢弃
                1 保留

    过滤逻辑：
        这里的过滤是白名单原则：
        1. 第1阶段，比对acttype，只要命中一条就可以进行第2阶段，全都没命中，则返回0，话单可以丢弃
        2. 第2阶段，依次比对any，both，from，to 中的号码规则，只要命中一条，返回1，全部规则未命中则返回0
*/
int filter_voipa_cdr(st_voipa_cdr_t *cdr, enum_voipa_cdr_type_t type)
{
    if(!cdr)
    {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, 
                "[%s:%d] invalid voipa_cdr",
                __FILE__, __LINE__);
        return 0;
    }

    if(type != VOIPA_TYPE_CASE && type != VOIPA_TYPE_MASS)
    {
        applog(APP_LOG_LEVEL_ERR, APP_LOG_MASK_BASE, 
                "[%s:%d] invalid voipa cdr type",
                __FILE__, __LINE__);
        return 0;
    }

    int stage_1 = 0;
    
    int acttype = type == VOIPA_TYPE_CASE ? atoi(cdr->voip_case.acttype) : atoi(cdr->voip_mass.acttype);
    char *aisdn = type == VOIPA_TYPE_CASE ? cdr->voip_case.aisdn : cdr->voip_mass.aisdn;
    char *bisdn = type == VOIPA_TYPE_CASE ? cdr->voip_case.bisdn : cdr->voip_mass.bisdn;
    char *from_id = (acttype == 0x302 || acttype == 0x304) ? bisdn : aisdn;
    char *  to_id = (acttype == 0x302 || acttype == 0x304) ? aisdn : bisdn;
    if(from_id[0] == '+') from_id ++;
    if(to_id[0] == '+') to_id ++;
    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[%s:%d] begin check cdr: acttype %d, from_id %s, to_id %s",
            __FILE__, __LINE__, acttype, from_id, to_id);

    //遍历acttype链表
    hlist_node_t * p = voipa_filter_rules->acttype_rules->first;
    while(p != NULL)
    {
        st_voipa_fltr_acttype_rule_t * rule = container_of(p, st_voipa_fltr_acttype_rule_t, node);
        if(rule->acttype == acttype)
        {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[cdr HIT acttype rule] : acttype = %d", acttype);
            stage_1 = 1;
            break;
        }
        
        p = p->next;
    }

    if(stage_1 == 0)
    {
        applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[cdr CANNOT HIT acttype-rule] acttype = %d", acttype);
        return 0;
    }

    p = voipa_filter_rules->isdn_any_rules->first;
    while(p != NULL)
    {
        st_voipa_fltr_isdn_rule_t * rule = container_of(p, st_voipa_fltr_isdn_rule_t, node);
        //any中的规则表示from_id 和 to_id其中之一匹配即可
        if((regexec(&rule->reg, from_id, 0, NULL, 0) == 0) || 
           (regexec(&rule->reg, to_id, 0, NULL, 0) == 0))
        {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[cdr HIT isdn ANY-rule] from_id: %s, to_id: %s, pattern: %s",
                    from_id, to_id, rule->reg_exp);
            return 1;
        }
        
        p = p->next;
    }

    p = voipa_filter_rules->isdn_both_rules->first;
    while(p != NULL)
    {
        st_voipa_fltr_isdn_rule_t * rule = container_of(p, st_voipa_fltr_isdn_rule_t, node);
        //both中的规则表示from_id 和 to_id都要匹配
        if((regexec(&rule->reg, from_id, 0, NULL, 0) == 0) && 
           (regexec(&rule->reg, to_id, 0, NULL, 0) == 0))
        {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[cdr HIT isdn BOTH-rule] from_id: %s, to_id: %s, pattern: %s",
                    from_id, to_id, rule->reg_exp);
            return 1;
        }
        
        p = p->next;
    }

    p = voipa_filter_rules->isdn_from_rules->first;
    while(p != NULL)
    {
        st_voipa_fltr_isdn_rule_t * rule = container_of(p, st_voipa_fltr_isdn_rule_t, node);
        //from中的规则表示from_id要匹配
        if(regexec(&rule->reg, from_id, 0, NULL, 0) == 0)
        {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[cdr HIT isdn FROM-rule] from_id: %s, pattern: %s",
                    from_id, rule->reg_exp);
            return 1;
        }
        
        p = p->next;
    }

    p = voipa_filter_rules->isdn_to_rules->first;
    while(p != NULL)
    {
        st_voipa_fltr_isdn_rule_t * rule = container_of(p, st_voipa_fltr_isdn_rule_t, node);
        //from中的规则表示from_id要匹配
        if(regexec(&rule->reg, to_id, 0, NULL, 0) == 0)
        {
            applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[cdr HIT isdn TO-rule] to_id: %s, pattern: %s",
                    to_id, rule->reg_exp);
            return 1;
        }
        
        p = p->next;
    }

    applog(APP_LOG_LEVEL_INFO, APP_LOG_MASK_BASE, "[cdr CANNOT HIT isdn rule] from_id: %s, to_id: %s",
            from_id, to_id);
            
    return 0;
}

