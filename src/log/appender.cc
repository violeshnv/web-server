#include "log.hh"

StdoutLogAppender::StdoutLogAppender(
    LogLevel level, LogFormatter::ptr formatter) :
    LogAppender(level, formatter) { }

void StdoutLogAppender::Log(LogInfo const& info, LogEvent::ptr event)
{
    if (info.Level() < level_) return;
    std::cout << formatter_->Format(info, event);
}

FileLogAppender::FileLogAppender(
    LogLevel level, LogFormatter::ptr formatter, std::string_view filename) :
    LogAppender(level, formatter),
    fstream_(std::string(filename))
{
    if (!fstream_.is_open()) std::cerr << "Failed to open " << filename << std::endl;
}

void FileLogAppender::Log(LogInfo const& info, LogEvent::ptr event)
{
    if (info.Level() < level_) return;
    // if (!fstream_.is_open()) fstream_.open(name_);
    fstream_ << formatter_->Format(info, event);
}

auto FileLogAppender::Reopen() -> bool
{
    if (fstream_) fstream_.close();
    fstream_.open(name_);
    return bool(fstream_);
}