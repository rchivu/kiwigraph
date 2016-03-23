#include <errno.h>

#include "platform.h"
#include "../logger.h"

int PWaitOnThread(PThreadID threadId, void** result) {
	return pthread_join(threadId, result);
}

int PStartThread(void* threadArg, StartThreadFunc startRoutine, PThreadID& threadId){
	int result = pthread_create(&threadId, NULL, startRoutine, threadArg);
	const Logger& logger = Logger::getLogger();

	switch(result) {
		case EAGAIN:{
			logger.error("Insufficient resources to create another thread");
			break;
		}
		case EINVAL:{
			logger.error("Invalid settings in attr");
			break;
		}
		case EPERM:{
			logger.error("No permission to set the scheduling policy and parameters"
						 "specified in attr.");
		}
	}
	return result;
}
