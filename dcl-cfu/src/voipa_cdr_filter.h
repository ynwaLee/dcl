#ifndef _VOIPA_CDR_FILTER_H_
#define _VOIPA_CDR_FILTER_H_
#include <sys/types.h>
#include <regex.h>

#include "../../common/lt_list.h"
#include "voipa_cdr.h"

typedef struct
{
    int acttype;
    hlist_node_t node;
}st_voipa_fltr_acttype_rule_t;

typedef struct
{
    char reg_exp[128];
    regex_t reg;
    hlist_node_t node;
}st_voipa_fltr_isdn_rule_t;

typedef struct
{
    hlist_head_t * acttype_rules;
    hlist_head_t * isdn_any_rules;
    hlist_head_t * isdn_both_rules;
    hlist_head_t * isdn_from_rules;
    hlist_head_t * isdn_to_rules;
}st_voipa_fltr_rules_t;

typedef enum
{
    VOIPA_TYPE_CASE,
    VOIPA_TYPE_MASS,
}enum_voipa_cdr_type_t;

int init_voipa_cdr_filter(void);
void destory_voipa_cdr_filter(void);
int filter_voipa_cdr(st_voipa_cdr_t *cdr, enum_voipa_cdr_type_t type);


#endif
