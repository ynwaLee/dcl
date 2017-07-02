
#include <signal.h>
#include <string.h>
#include "thread_flag.h"

static void signal_catch(int sig, void (*handler)(int))
{
    struct sigaction action;
    memset(&action, 0x00, sizeof(struct sigaction));

    action.sa_handler = handler;
    sigemptyset(&(action.sa_mask));
    sigaddset(&(action.sa_mask),sig);
    action.sa_flags = 0;
    sigaction(sig, &action, 0);

    return;
}


void init_signal_catch(void)
{
    signal_catch(SIGINT, signal_handler_sigint);
    signal_catch(SIGTERM, signal_handler_sigterm);
    signal_catch(SIGPIPE, SIG_IGN);
}

