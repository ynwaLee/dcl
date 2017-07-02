

#include <pthread.h>

#define Mutex pthread_mutex_t
#define MutexAttr pthread_mutexattr_t
#define MutexDestroy pthread_mutex_destroy
#define MutexInit(mut, mutattr ) pthread_mutex_init(mut, mutattr)
#define MutexLock(mut) pthread_mutex_lock(mut)
#define MutexTrylock(mut) pthread_mutex_trylock(mut)
#define MutexUnlock(mut) pthread_mutex_unlock(mut)

