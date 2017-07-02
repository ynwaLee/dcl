



#define THREAD_RUN_STOP    (1 << 0)
#define THREAD_RUN_KILL    (1 << 1)
#define THREAD_RUN_DONE    (1 << 2)

#define THREAD_OPR_FLAG_IDLE    0
#define THREAD_OPR_FLAG_REQ        1
#define THREAD_OPR_FLAG_OK        2

extern unsigned char svm_signal_flags;

void signal_handler_sigint(__attribute__((unused))int sig);
void signal_handler_sigterm(__attribute__((unused))int sig);


