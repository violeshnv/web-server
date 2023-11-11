#include "log.hh"

Logger::Logger(LogLevel level, std::string_view name) :
    level_(level), name_(name), appenders_() { }

void Logger::Log(LogLevel level, LogEvent::ptr event)
{
    if (level < level_) return;
    for (auto& p_app : appenders_) {
        p_app->Log(LogInfo(this, level), event);
    }
}

void Logger::Debug(LogEvent::ptr event) { Log(LogLevel::DEBUG, event); }
void Logger::Info(LogEvent::ptr event) { Log(LogLevel::INFO, event); }
void Logger::Warn(LogEvent::ptr event) { Log(LogLevel::WARN, event); }
void Logger::Error(LogEvent::ptr event) { Log(LogLevel::ERROR, event); }
void Logger::Fatal(LogEvent::ptr event) { Log(LogLevel::FATAL, event); }

void Logger::AddAppender(LogAppender::ptr appender)
{
    for (auto it = appenders_.begin(); it != appenders_.end(); ++it) {
        if (*it == appender) return;
    }
    appenders_.emplace_front(appender);
}

void Logger::DelAppender(LogAppender::ptr appender)
{
    for (auto it = appenders_.begin(); it != appenders_.end(); ++it) {
        if (*it == appender) appenders_.erase(it);
        break;
    }
}
