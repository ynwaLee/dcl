#ifndef __COUNTER_H__
#define __COUNTER_H__

#include "atomic.h"


#define MAX_BCP_NUM     100000
typedef struct all_count_ {
    uint64_t svm_file_num;
    uint64_t svm_call_num;
    uint64_t svm_mass_file_num;
    uint64_t svm_mass_call_num;
    uint64_t svm_case_file_num;
    uint64_t svm_case_call_num;
    uint64_t mobile_file_num;
    uint64_t mobile_call_num;
    uint64_t mobile_mass_file_num;
    uint64_t mobile_mass_call_num;
    uint64_t mobile_case_file_num;
    uint64_t mobile_case_call_num;
    uint64_t voipa_file_num;
    uint64_t voipa_call_num;
    uint64_t voipa_mass_file_num;
    uint64_t voipa_mass_call_num;
    uint64_t voipa_case_file_num;
    uint64_t voipa_case_call_num;
    uint64_t voipb_file_num;
    uint64_t voipb_call_num;
    uint64_t voipb_mass_file_num;
    uint64_t voipb_mass_call_num;
    uint64_t voipb_case_file_num;
    uint64_t voipb_case_call_num;
    uint64_t das_file_num;
    uint64_t das_call_num;
    uint64_t das_mass_file_num;
    uint64_t das_mass_call_num;
    uint64_t das_case_file_num;
    uint64_t das_case_call_num;
    uint64_t das_upload_success;
} all_count_t;



extern atomic64_t svm_file_num;
extern atomic64_t svm_call_num;
extern atomic64_t svm_mass_file_num;
extern atomic64_t svm_mass_call_num;
extern atomic64_t svm_case_file_num;
extern atomic64_t svm_case_call_num;
extern atomic64_t mobile_file_num;
extern atomic64_t mobile_call_num;
extern atomic64_t mobile_mass_file_num;
extern atomic64_t mobile_mass_call_num;
extern atomic64_t mobile_case_file_num;
extern atomic64_t mobile_case_call_num;
extern atomic64_t voipa_file_num;
extern atomic64_t voipa_call_num;
extern atomic64_t voipa_mass_file_num;
extern atomic64_t voipa_mass_call_num;
extern atomic64_t voipa_case_file_num;
extern atomic64_t voipa_case_call_num;
extern atomic64_t voipb_file_num;
extern atomic64_t voipb_call_num;
extern atomic64_t voipb_mass_file_num;
extern atomic64_t voipb_mass_call_num;
extern atomic64_t voipb_case_file_num;
extern atomic64_t voipb_case_call_num;

extern atomic64_t das_file_num;
extern atomic64_t das_call_num;
extern atomic64_t das_mass_file_num;
extern atomic64_t das_mass_call_num;
extern atomic64_t das_case_file_num;
extern atomic64_t das_case_call_num;
extern atomic64_t das_upload_success;


void init_atomic_counter(void);

#endif
