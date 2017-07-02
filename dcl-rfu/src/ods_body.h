
#ifndef __ODS_BODY_H__
#define __ODS_BODY_H__

#include "variable.h"


unsigned int gi_debug_ods_malloc_numbler;
unsigned int gi_debug_ods_free_numbler;
unsigned int gi_debug_ods_cpy_numbler;

#define ods_length (sizeof(unsigned int))
#define ods_malloc(size) \
    ({\
        void *pv_data = NULL;\
        if( size>0 )\
        {\
            pv_data = MY_MALLOC(size+ods_length);\
            *(unsigned int *)(pv_data)=1;\
            gi_debug_ods_malloc_numbler++;\
        }\
        else\
        {\
            syslog (LOG_ERR, "in the %s function, call malloc but size is zero, exit proccess!", __FUNCTION__);\
            exit (-1);\
        }\
        (pv_data+ods_length);\
     })

#define ods_free(pv_data) \
    ({\
        void *pv_src = (void *)(pv_data);\
        if( pv_data!=NULL )\
        {\
            gi_debug_ods_free_numbler++;\
            unsigned int *pui_len = (unsigned int *)(pv_src-ods_length);\
            if(*pui_len==0 || *pui_len==1)\
            {\
                free((pv_src-ods_length));\
                pv_data = NULL;\
            }\
            else\
            {\
                (*pui_len)--;\
                pv_data = NULL;\
            }\
        }\
     })

#define ods_memcpy(pv_data) ({ gi_debug_ods_cpy_numbler++; void *pv_src = (void *)(pv_data); ((*(unsigned int *)(pv_src-ods_length)))++; pv_src; })

#define ods_get_num(pv_src) (*(unsigned int *)(((void *)(pv_src))-ods_length))


#endif


