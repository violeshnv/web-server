#ifndef __LOG_CONFIG__H_
#define __LOG_CONFIG__H_

#include "config.hh"
#include "log/log.hh"

namespace YAML
{
template <>
struct convert<LogLevel> {
    static Node encode(LogLevel const& level)
    {
        return Node(std::string(level));
    }
    static bool decode(Node const& node, LogLevel& level)
    {
        if (!node.IsScalar()) return false;

        auto v = magic_enum::enum_cast<decltype(LogLevel::UNKNOWN)>(
            node.as<std::string_view>());
        if (!v) return false;

        level = v.value();
        return true;
    }
};

template <>
struct convert<LogFormatter::ptr> {
    static Node encode(LogFormatter::ptr const& formatter)
    {
        Node node;
        node = formatter->Pattern();
        return node;
    }
    static bool decode(Node const& node, LogFormatter::ptr& formatter)
    {
        if (!node.IsScalar()) return false;
        formatter = std::make_shared<LogFormatter>(node.as<std::string_view>());
        return true;
    }
};

template <>
struct convert<LogAppender::ptr> {
    static Node encode(LogAppender::ptr const& appender)
    {
        Node node;
        auto& manager = LogManager::Instance();

        node["level"] = appender->Level();
        node["format"] = manager.GetName(appender->Formatter());

        return node;
    }
    static bool decode(Node const& node, LogAppender::ptr& appender)
    {
        if (!node.IsMap()) return false;
        auto& manager = LogManager::Instance();

        auto level = node["level"].as<LogLevel>();
        auto format = manager.GetFormatter(
            node["format"].as<std::string_view>());

        auto name = node["name"].as<std::string_view>();
        if (name.starts_with("stdout")) {
            appender = std::make_shared<StdoutLogAppender>(
                level, format);
        } else if (name.starts_with("file")) {
            appender = std::make_shared<FileLogAppender>(
                level, format, node["filename"].as<std::string_view>());
        } else return false;
        return true;
    }
};

template <>
struct convert<Logger::ptr> {
    static Node encode(Logger::ptr const& logger)
    {
        Node v;
        auto& manager = LogManager::Instance();

        for (auto const& app : logger->Appenders()) {
            v.push_back(manager.GetName(app));
        }

        Node node;
        node["name"] = logger->Name();
        node["level"] = logger->Level();
        node["appenders"] = v;

        return node;
    }
    static bool decode(Node const& node, Logger::ptr& logger)
    {
        if (!node.IsMap()) return false;
        auto& manager = LogManager::Instance();
        logger = Logger::ptr(new Logger(
            node["level"].as<LogLevel>(),
            node["name"].as<std::string_view>()));
        for (auto& n : node["appenders"]) {
            logger->AddAppender(manager.GetAppender(n.as<std::string_view>()));
        }
        return true;
    }
};
} // namespace YAML

auto LogInit(YAML::Node const& node) -> bool
{
    if (!node.IsMap() || !node["log"]) return false;

    auto& manager = LogManager::Instance();

    auto log = node["log"];

    for (auto const& [k, v] : log["format"].as<std::map<std::string_view, LogFormatter::ptr>>()) {
        manager.AddFormat(k, v);
    }

    for (auto const& a : log["appender"]) {
        auto app = a.as<LogAppender::ptr>();
        manager.AddAppender(a["name"].as<std::string_view>(), app);
    }

    for (auto const& l : log["logger"]) {
        auto logger = l.as<Logger::ptr>();
        manager.AddLogger(logger);
    }

    return true;
}

#endif // __LOG_CONFIG__H_