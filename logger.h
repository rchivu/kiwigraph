#ifndef LOGGER_H
#define LOGGER_H

#include <string>

enum LogLevel {
	LogLevel_Debug,
	LogLevel_Warn,
	LogLevel_Error
};

class Logger {
public:
	static void setGlobalLoggingLevel(LogLevel level);
	static const Logger& getLogger();
	virtual ~Logger() {}
	virtual void debug(const std::string& message) const = 0;
	virtual void warn(const std::string& message) const = 0;
	virtual void error(const std::string& message) const = 0;
	virtual void setLoggingLevel(LogLevel level) = 0;
};

#endif // LOGGER_H
