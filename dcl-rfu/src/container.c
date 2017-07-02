
#include <stdlib.h>
#include <string.h>

#include "variable.h"
#include "container.h"

container* create_container(void)
{
    container *pst_var = (container *)MY_MALLOC(sizeof(container));    
    pst_var->head = (con_data_st *)MY_MALLOC(sizeof(con_data_st));
    pst_var->ui_total = 0;
    return pst_var;
}

int container_add_data(container *pst_src, void *data)
{
    if( pst_src==NULL || data==NULL )
    {
        return -1;
    }
    pst_src->ui_total++;
    con_data_st *pst_head = pst_src->head;
    con_data_st *pst_var = (con_data_st *)MY_MALLOC(sizeof(con_data_st));
    con_data_st *pst_tmp = pst_head->next;
    pst_var->data = data;
    
    pst_head->next = pst_var;    
    pst_var->back = pst_head;    
    pst_var->next = pst_tmp;    
    if( pst_tmp!=NULL )
    {
        pst_tmp->back = pst_var;
    }

    return 0;
}

// Use in the for loop.
int container_free_node(container *pst_src)
{
    if( pst_src==NULL )
    {
        return -1;
    }
    if( pst_src->now==NULL )
    {
        return -1;
    }
    pst_src->ui_total--;
    con_data_st *pst_now = pst_src->now;
    con_data_st *pst_fist = pst_now->back;
    con_data_st *pst_lass = pst_now->next;
        
    pst_fist->next = pst_lass;
    if( pst_lass!=NULL )
    {
        pst_lass->back = pst_fist;    
    }
    pst_src->now = pst_fist;

    return 0;
}

//strcat, move
int container_catenate(container *dest, container *src)
{
    if( dest==NULL || src==NULL )
    {
        return -1;
    }
    
    if( container_is_null(src) )
    {
        return 0;
    }
    
    dest->ui_total += src->ui_total;
    src->ui_total = 0;
    con_data_st *pst_dest_end = dest->head;
    con_data_st *pst_src_head = src->head->next;
    
    while( pst_dest_end->next!=NULL )
    {
        pst_dest_end = pst_dest_end->next;
    }
    
    pst_dest_end->next = pst_src_head;        
    pst_src_head->back = pst_dest_end;
    
    src->head->next = NULL;

    return 0;
}

int container_destory(container *pst_src)
{
    if( pst_src==NULL )
    {
        return -1;
    }

    con_data_st *pst_head = pst_src->head;    
    con_data_st *pst_tmp = NULL;
    while( pst_head!=NULL )
    {
        pst_tmp = pst_head;
        pst_head = pst_head->next;
        MY_FREE (pst_tmp);
    }
    MY_FREE(pst_src);

    return 0;
}




