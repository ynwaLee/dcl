

#include "thread_flag.h"

unsigned char svm_signal_flags = 0;



void signal_handler_sigint(__attribute__((unused))int sig)
{
    svm_signal_flags |= THREAD_RUN_STOP;
}

void signal_handler_sigterm(__attribute__((unused))int sig)
{
    svm_signal_flags |= THREAD_RUN_KILL;
}

