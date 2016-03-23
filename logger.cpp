#include <memory>
#include <cstdio>
#include "logger.h"

namespace {
const char* getLogLevelMessage(LogLevel level){
	switch(level) {
		case LogLevel_Debug: return "DEBUG";
		case LogLevel_Error: return "ERROR";
		case LogLevel_Warn: return "WARN";
		default: return "DEBUG";
	}
}

std::unique_ptr<Logger> loggerInstance;
class LoggerImpl: public Logger {
private:
	LogLevel m_currentLevel;
	void print(LogLevel level, const std::string& message) const {
		if(level > m_currentLevel) {
			const char* levelMessage = getLogLevelMessage(level);
			std::printf("%s: %s", levelMessage, message.c_str());
		}
	}
public:
	virtual void debug(const std::string& message) const;
	virtual void warn(const std::string& message) const;
	virtual void error(const std::string& message) const;
	virtual void setLoggingLevel(LogLevel level);
};
}

void LoggerImpl::debug(const std::string& message) const {
	print(LogLevel_Debug, message);
}
void LoggerImpl::warn(const std::string& message) const {
	print(LogLevel_Warn, message);
}
void LoggerImpl::error(const std::string& message) const {
	print(LogLevel_Error, message);
}
void LoggerImpl::setLoggingLevel(LogLevel level) {
	m_currentLevel = level;
}

const Logger& Logger::getLogger(){
	if(loggerInstance.get() == nullptr){
		loggerInstance.reset(new LoggerImpl());
	}
	return *loggerInstance.get();
}

void Logger::setGlobalLoggingLevel(LogLevel level){
	loggerInstance->setLoggingLevel(level);
}

