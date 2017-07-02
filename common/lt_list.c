#include "lt_list.h"
#include <stdlib.h>

hlist_head_t * hlist_create(void)
{
    hlist_head_t *head = (hlist_head_t *)malloc(sizeof(hlist_head_t));
    head->first = NULL;
    head->last = NULL;
    head->node_cnt = 0;

    return head;
}
unsigned int hlist_get_len(hlist_head_t *head)
{
    return head->node_cnt;
}

int hlist_insert_tail(hlist_head_t *head, hlist_node_t *node)
{
    hlist_node_t *p = head->last;
    head->last = node;
    if(p != NULL)
    {
        p->next = node;
        node->next = NULL;
        node->pprev = &p->next;
    }
    else
    {
        head->first = node;
        head->last = node;
        node->pprev = &head->first;
    }

    head->node_cnt ++;
    
    return 0;
}

int hlist_insert_head(hlist_head_t *head, hlist_node_t *node)
{
    if(!head->first)
    {
        head->last = node;
    }
    
    node->next = head->first;
    head->first = node;
    node->pprev = &head->first;
    if(node->next)
    {
        node->next->pprev = &node->next;
    }
    
    head->node_cnt ++;
    
    return 0;
}

int hlist_destroy(hlist_head_t * head)
{
    //TODO
    (void)head;
    return 0;
}

hlist_node_t * hlist_get_head_node(hlist_head_t *head)
{
    if(head->node_cnt == 0)
    {
        return NULL;
    }
    
    hlist_node_t * p = head->first;
    *(p->pprev) = p->next;

    if(p->next)
    {
        p->next->pprev = p->pprev;
    }

    head->node_cnt --;
    if(head->node_cnt == 0)
    {
        head->last = NULL;
    }
    
    return p;
}

hlist_node_t * hlist_get_tail_node(hlist_head_t *head)
{
    if(head->node_cnt == 0)
    {
        return NULL;
    }

    hlist_node_t * p = head->last;

    head->last = (hlist_node_t *)p->pprev;
    *(p->pprev) = p->next;
    head->node_cnt --;

    return p;
}

int hlist_delete_node(hlist_head_t *head, hlist_node_t * node)
{
    *(node->pprev) = node->next;
    if(node->next)
    {
        node->next->pprev = node->pprev;
    }

    head->node_cnt --;

    return 0;
}

