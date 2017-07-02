
#ifndef __CONTAINER_H__
#define __CONTAINER_H__


typedef struct con_data
{
    struct con_data *next;
    struct con_data *back;
    void *data;
}con_data_st;

typedef struct contain
{
    con_data_st *head;    
    con_data_st *now;
    unsigned int ui_total;
}container;


#define get_container_value(container_var) \
({\
    void *tmp = NULL;\
    if(container_var->now!=NULL)\
    {\
        tmp = container_var->now->data;    \
    }\
    tmp;\
 })

#define container_search(macro_var, macro_container) \
    for ((macro_container)->now=(macro_container)->head->next;\
                ((macro_var)=(typeof(macro_var))(get_container_value(macro_container)));\
                (macro_container)->now=(macro_container)->now->next)

#define container_is_null(macro_container) (!macro_container->head->next)

#define container_node_num(macro_container) ((macro_container)->ui_total)

extern container* create_container(void);
extern int container_add_data(container *pst_src, void *data);
extern int container_free_node(container *pst_src);
extern int container_destory(container *pst_src);
extern int container_catenate(container *dest, container *src);

#endif


