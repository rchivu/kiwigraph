#ifndef PLATFORM_H
#define PLATFORM_H

#include <pthread.h>
typedef pthread_t PThreadID;
typedef void* (*StartThreadFunc) (void* arg);

int PWaitOnThread(PThreadID threadId, void** result);
int PStartThread(void* threadArg, StartThreadFunc, PThreadID& threadId);

#endif // PLATFORM_H
