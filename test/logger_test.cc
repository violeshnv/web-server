#include "log/log.hh"

int main()
{
    auto p = std::make_shared<Logger>(LogLevel::DEBUG, "test");

    auto formatter = std::make_shared<LogFormatter>(
        "%%%n "
        "log name: %c%n "
        "message: %m%n "
        "level: %p%n "
        "functiona name: %x%n "
        "thread id: %t{%Y:%m:%d %H:%M:%S}%n "    // ignore format
        "elapsed time: %r{%Y:%m:%d %H:%M:%S}%n " // ignore format
        "time: %d{%Y:%m:%d %H:%M:%S}%n "
        "line feed: %n%n "
        "tab: %T%n "
        "file name: %f%n "
        "file line number: %l%n ");

    p->AddAppender(std::make_shared<StdoutLogAppender>(LogLevel::INFO, formatter));
    p->AddAppender(std::make_shared<FileLogAppender>(LogLevel::DEBUG, formatter, "log.log"));

    auto e = std::make_shared<LogEvent>(
        std::source_location::current().file_name(),
        std::source_location::current().function_name(),
        std::source_location::current().line(),
        std::this_thread::get_id(), 0,
        time(0), time(0), "test content");

    p->Log(LogLevel::WARN, e);
}