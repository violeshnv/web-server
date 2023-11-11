#include "log.hh"

LogInfo::LogInfo(Logger const* logger, LogLevel level) :
    logger_name_(logger->Name()), level_(level) { }