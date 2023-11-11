#include "log.hh"

void LogManager::AddFormat(std::string_view name, LogFormatter::ptr formatter)
{
    for (auto& [n, f] : formatters_) {
        if (n == name) {
            f = formatter;
            return;
        }
    }
    formatters_.emplace_back(name, formatter);
}

void LogManager::DelFormat(std::string_view name)
{
    size_t i = 0;
    for (auto const& [n, _] : formatters_) {
        if (n == name) break;
        ++i;
    }
    if (i == formatters_.size()) return;

    std::swap(formatters_[i], formatters_.back());
    formatters_.pop_back();
}

void LogManager::AddAppender(std::string_view name, LogAppender::ptr appender)
{
    for (auto& [n, a] : appenders_) {
        if (n == name) {
            a = appender;
            return;
        }
    }
    appenders_.emplace_back(name, appender);
}

void LogManager::DelAppender(std::string_view name)
{
    size_t i = 0;
    for (auto const& [n, _] : appenders_) {
        if (n == name) break;
        ++i;
    }
    if (i == appenders_.size()) return;

    std::swap(appenders_[i], appenders_.back());
    appenders_.pop_back();
}

void LogManager::AddLogger(Logger::ptr logger)
{
    loggers_.emplace(logger->Name(), logger);
}

void LogManager::DelLogger(Logger::ptr logger)
{
    loggers_.erase(logger->Name());
}

auto LogManager::GetFormatter(std::string_view name) const -> LogFormatter::ptr
{
    for (auto& [n, f] : formatters_) {
        if (n == name) {
            return f;
        }
    }
    return nullptr;
}
auto LogManager::GetAppender(std::string_view name) const -> LogAppender::ptr
{
    for (auto& [n, a] : appenders_) {
        if (n == name) {
            return a;
        }
    }
    return nullptr;
}
auto LogManager::GetLogger(std::string_view name) const -> Logger::ptr
{
    if (auto find = loggers_.find(name); find != loggers_.end()) {
        return find->second;
    }
    return nullptr;
}

auto LogManager::GetName(LogFormatter::ptr formatter) const
    -> const std::string_view
{
    for (auto& [n, f] : formatters_) {
        if (f == formatter) {
            return n;
        }
    }
    return std::string_view();
}
auto LogManager::GetName(LogAppender::ptr appender) const
    -> const std::string_view
{
    for (auto& [n, a] : appenders_) {
        if (a == appender) {
            return n;
        }
    }
    return std::string_view();
}
auto LogManager::GetName(Logger::ptr logger) const
    -> const std::string_view
{
    if (logger) return logger->Name();
    return std::string_view();
}