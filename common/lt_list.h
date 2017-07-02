#ifndef _LT_LIST_H_
#define _LT_LIST_H_

typedef struct hlist_node
{
    struct hlist_node *next;
    struct hlist_node **pprev;
}hlist_node_t;

typedef struct hlist_head
{
    hlist_node_t *first;
    hlist_node_t *last;
    unsigned int node_cnt;
}hlist_head_t;

hlist_head_t * hlist_create(void);
unsigned int hlist_get_len(hlist_head_t *head);
int hlist_insert_tail(hlist_head_t *head, hlist_node_t *node);
int hlist_insert_head(hlist_head_t *head, hlist_node_t *node);
hlist_node_t * hlist_get_head_node(hlist_head_t *head);
hlist_node_t * hlist_get_tail_node(hlist_head_t *head);
int hlist_delete_node(hlist_head_t *head, hlist_node_t * node);
int hlist_destroy(hlist_head_t *);

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE*)0)->MEMBER)

#define container_of(ptr, type, member) \
({ \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) ); \
})  

#endif
