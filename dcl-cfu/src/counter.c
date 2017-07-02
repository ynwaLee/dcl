#include "atomic.h"
#include "counter.h"


atomic64_t svm_file_num;
atomic64_t svm_call_num;
atomic64_t svm_mass_file_num;
atomic64_t svm_mass_call_num;
atomic64_t svm_case_file_num;
atomic64_t svm_case_call_num;
atomic64_t mobile_file_num;
atomic64_t mobile_call_num;
atomic64_t mobile_mass_file_num;
atomic64_t mobile_mass_call_num;
atomic64_t mobile_case_file_num;
atomic64_t mobile_case_call_num;
atomic64_t voipa_file_num;
atomic64_t voipa_call_num;
atomic64_t voipa_mass_file_num;
atomic64_t voipa_mass_call_num;
atomic64_t voipa_case_file_num;
atomic64_t voipa_case_call_num;
atomic64_t voipb_file_num;
atomic64_t voipb_call_num;
atomic64_t voipb_mass_file_num;
atomic64_t voipb_mass_call_num;
atomic64_t voipb_case_file_num;
atomic64_t voipb_case_call_num;

atomic64_t das_file_num;
atomic64_t das_call_num;
atomic64_t das_mass_file_num;
atomic64_t das_mass_call_num;
atomic64_t das_case_file_num;
atomic64_t das_case_call_num;
atomic64_t das_upload_success;



void init_atomic_counter(void)
{
    atomic64_init(&svm_file_num);
    atomic64_init(&svm_call_num);
    atomic64_init(&svm_mass_file_num);
    atomic64_init(&svm_mass_call_num);
    atomic64_init(&svm_case_file_num);
    atomic64_init(&svm_case_call_num);
    atomic64_init(&mobile_file_num);
    atomic64_init(&mobile_call_num);
    atomic64_init(&mobile_mass_file_num);
    atomic64_init(&mobile_mass_call_num);
    atomic64_init(&mobile_case_file_num);
    atomic64_init(&mobile_case_call_num);
    atomic64_init(&voipa_file_num);
    atomic64_init(&voipa_call_num);
    atomic64_init(&voipa_mass_file_num);
    atomic64_init(&voipa_mass_call_num);
    atomic64_init(&voipa_case_file_num);
    atomic64_init(&voipa_case_call_num);
    atomic64_init(&voipb_file_num);
    atomic64_init(&voipb_call_num);
    atomic64_init(&voipb_mass_file_num);
    atomic64_init(&voipb_mass_call_num);
    atomic64_init(&voipb_case_file_num);
    atomic64_init(&voipb_case_call_num);
    atomic64_init(&das_file_num);
    atomic64_init(&das_call_num);
    atomic64_init(&das_mass_file_num);
    atomic64_init(&das_mass_call_num);
    atomic64_init(&das_case_file_num);
    atomic64_init(&das_case_call_num);
    atomic64_init(&das_upload_success);
}


