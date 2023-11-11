#ifndef __LOG__H_
#define __LOG__H_

#include <cassert>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <stdexcept>

#include <fstream>
#include <iomanip>
#include <iostream>

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <type_traits>

#include <list>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "magic_enum.hh"

#include "fiber/fiber.hh"

class Logger;
class LogAppender;
class LogManager;

class LogLevel
{
public:
    enum : int {
        UNKNOWN = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };

private:
    typedef decltype(LogLevel::UNKNOWN) Level;

public:
    constexpr LogLevel() :
        LogLevel(LogLevel::UNKNOWN) { }
    constexpr LogLevel(Level level) :
        level_(level) { }

    constexpr operator std::string_view() const
    {
        auto name = magic_enum::enum_name(level_);
        return name ? name.value() : "<<WHY DID YOU TRY TO REACH HERE?>>";
    }
    constexpr operator int() const { return static_cast<int>(level_); }

private:
    Level level_;
};

class LogEvent
{
public:
    typedef LogEvent self;
    typedef std::shared_ptr<self> ptr;

    template <typename U>
    LogEvent(std::string_view file,
             std::string_view func_name,
             int line,
             std::thread::id thread_id,
             Fiber::id_t fiber_id,
             time_t elapsed_time,
             time_t time,
             U&& content) :
        file_(file),
        func_name_(func_name),
        line_(line),
        thread_id_(thread_id),
        fiber_id_(fiber_id),
        elapsed_time_(elapsed_time),
        time_(time),
        content_(std::forward<U>(content)) { }

    auto const& File() const { return file_; }
    auto const& FuncName() const { return func_name_; }
    auto Line() const { return line_; }
    auto ThreadId() const { return thread_id_; }
    auto FiberId() const { return fiber_id_; }
    auto ElapsedTime() const { return elapsed_time_; }
    auto Time() const { return time_; }
    auto const& Content() const { return content_; }

private:
    std::string file_;
    std::string func_name_;
    int line_;
    std::thread::id thread_id_;
    Fiber::id_t fiber_id_;
    time_t elapsed_time_;
    time_t time_;
    std::string content_;
};

class LogInfo
{
public:
    LogInfo(Logger const* logger, LogLevel level);

    auto const& Name() const { return logger_name_; }
    auto const& Level() const { return level_; }

private:
    std::string const& logger_name_;
    LogLevel level_;
};

class LogFormatter
{
public:
    struct Item {
        typedef Item self;
        typedef std::shared_ptr<self> ptr;
        virtual ~Item() = default;
        virtual void Format(
            std::ostream& os, LogInfo const& info, LogEvent::ptr event) = 0;

    protected:
        friend class LogFormatter;
        Item() = default;
    };

public:
    typedef LogFormatter self;
    typedef std::shared_ptr<self> ptr;

    LogFormatter(std::string_view pattern = default_pattern);

    auto const& Pattern() const { return pattern_; }

    auto Format(LogInfo const& info, LogEvent::ptr event) -> std::string;

private:
    auto ParsePattern()
        -> std::vector<std::pair<std::string, std::optional<std::string>>>;
    void ItemGen(
        std::vector<std::pair<std::string, std::optional<std::string>>>&&);

private:
    static constexpr char const* default_pattern = "[message: %m] [level: %p] [thread id: %t] [time: %d{%Y:%m:%d %H:%M:%S}]";

    std::string pattern_;
    std::vector<Item::ptr> items_;
};

class LogAppender
{
public:
    typedef LogAppender self;
    typedef std::shared_ptr<self> ptr;

    LogAppender(LogLevel level, LogFormatter::ptr formatter) :
        level_(level), formatter_(formatter) { }

    virtual ~LogAppender() = default;

    virtual void Log(LogInfo const& info, LogEvent::ptr event) = 0;

    auto const& Level() const { return level_; }
    auto Formatter() { return formatter_; }
    void SetLevel(LogLevel level) { level_ = level; }
    void SetFormatter(LogFormatter::ptr formatter) { formatter_ = formatter; }

protected:
    LogLevel level_;
    LogFormatter::ptr formatter_;
};

class Logger
{
public:
    typedef Logger self;
    typedef std::shared_ptr<self> ptr;

    Logger() :
        Logger(LogLevel::DEBUG) { }
    Logger(LogLevel level, std::string_view name = default_name);

    void Log(LogLevel level, LogEvent::ptr event);

    void Debug(LogEvent::ptr event);
    void Info(LogEvent::ptr event);
    void Warn(LogEvent::ptr event);
    void Error(LogEvent::ptr event);
    void Fatal(LogEvent::ptr event);

    void AddAppender(LogAppender::ptr appender);
    void DelAppender(LogAppender::ptr appender);

    auto const& Level() const { return level_; }
    void SetLevel(LogLevel level) { level_ = level; }

    auto const& Name() const { return name_; }
    auto const& Appenders() const { return appenders_; }

private:
    static constexpr char const* default_name = "root";

    LogLevel level_;
    std::string name_;
    std::string format_;
    std::list<LogAppender::ptr> appenders_;
};

#include "instance/instance.hh"

class LogManager
{
public:
    typedef LogManager self;
    typedef std::unique_ptr<self> ptr;

    void AddFormat(std::string_view name, LogFormatter::ptr formatter);
    void DelFormat(std::string_view name);
    void AddAppender(std::string_view name, LogAppender::ptr appender);
    void DelAppender(std::string_view name);
    void AddLogger(Logger::ptr logger);
    void DelLogger(Logger::ptr logger);

    auto GetFormatter(std::string_view name) const -> LogFormatter::ptr;
    auto GetAppender(std::string_view name) const -> LogAppender::ptr;
    auto GetLogger(std::string_view name) const -> Logger::ptr;
    auto GetName(LogFormatter::ptr formatter) const -> const std::string_view;
    auto GetName(LogAppender::ptr appender) const -> const std::string_view;
    auto GetName(Logger::ptr logger) const -> const std::string_view;

    auto const& Formatters() const { return formatters_; }
    auto const& Appenders() const { return appenders_; }
    auto const& Loggers() const { return loggers_; }
    auto const& StartTime() const { return start_time_; }

    LogManager() = default;
    LogManager(self const& other) = delete;
    LogManager(self&&) = delete;
    auto operator=(self&&) = delete;
    auto operator=(self const&) = delete;

private:
    std::vector<std::pair<std::string, LogFormatter::ptr>> formatters_;
    std::vector<std::pair<std::string, LogAppender::ptr>> appenders_;
    std::map<std::string_view, Logger::ptr> loggers_;

    time_t start_time_ = ::time(nullptr);

public:
    static auto& Instance()
    {
        static LogManager manager;
        return manager;
    }

    static void Log(LogLevel level, LogEvent::ptr event)
    {
        for (auto const& logger : Instance().Loggers() | std::views::values) {
            std::cout << logger->Name() << std::endl;
            logger->Log(level, event);
        }
    }
    static void Debug(LogEvent::ptr event) { Log(LogLevel::DEBUG, event); }
    static void Info(LogEvent::ptr event) { Log(LogLevel::INFO, event); }
    static void Warn(LogEvent::ptr event) { Log(LogLevel::WARN, event); }
    static void Error(LogEvent::ptr event) { Log(LogLevel::ERROR, event); }
    static void Fatal(LogEvent::ptr event) { Log(LogLevel::FATAL, event); }
};

// ----------------------
//  LogAppender Subclass
// ----------------------

class StdoutLogAppender : public LogAppender
{
public:
    typedef StdoutLogAppender self;
    typedef std::shared_ptr<self> ptr;

    StdoutLogAppender(LogLevel level, LogFormatter::ptr formatter);

    virtual void Log(LogInfo const& info, LogEvent::ptr event)
        override;

private:
};

class FileLogAppender : public LogAppender
{
public:
    typedef FileLogAppender self;
    typedef std::shared_ptr<self> ptr;

    FileLogAppender(LogLevel level, LogFormatter::ptr formatter,
                    std::string_view filename);

    auto const& FileName() const { return name_; }

    virtual void Log(LogInfo const& info, LogEvent::ptr event)
        override;

    auto Reopen() -> bool;

private:
    std::string name_;
    std::ofstream fstream_;
};

#include <source_location>

#define LOG(level, content)                                                              \
    LogManager::Instance().Log(level,                                                    \
                               LogEvent::ptr {new LogEvent(                              \
                                   std::source_location::current().file_name(),          \
                                   std::source_location::current().function_name(),      \
                                   std::source_location::current().line(),               \
                                   std::this_thread::get_id(), Fiber::GetId(),           \
                                   ::time(nullptr) - LogManager::Instance().StartTime(), \
                                   ::time(nullptr), content)})

#define LOG_DEBUG(content) LOG(LogLevel::DEBUG, content)
#define LOG_INFO(content)  LOG(LogLevel::INFO, content)
#define LOG_WARN(content)  LOG(LogLevel::WARN, content)
#define LOG_ERROR(content) LOG(LogLevel::ERROR, content)
#define LOG_FATAL(content) LOG(LogLevel::FATAL, content)

#endif // __LOG__H_