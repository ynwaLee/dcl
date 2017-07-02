#ifndef __RFU_CLIENT_H__
#define __RFU_CLIENT_H__


#define MAX_OBJECT_CLUE    10240

typedef struct object_clue_ {
    unsigned int flag;
    unsigned int object_id;
    unsigned int clue_id;
} object_clue_t;

void register_rfu_server(void);

unsigned int get_object(unsigned int clue);

#endif
