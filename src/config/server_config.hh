#ifndef __SERVER_CONFIG__H_
#define __SERVER_CONFIG__H_

#include "config.hh"
#include "server/server.hh"
#include "thread/thread.hh"

#include "instance/instance.hh"

namespace YAML
{
template <>
struct convert<ThreadPool::ptr> {
    static Node encode(ThreadPool::ptr const& pool)
    {
        Node node;
        node["count"] = pool->Count();
        return node;
    }
    static bool decode(Node const& node, ThreadPool::ptr& pool)
    {
        if (!node.IsMap()) return false;
        pool = ThreadPool::ptr(new ThreadPool(node["count"].as<size_t>()));
        return true;
    }
};
} // namespace YAML

auto ServerInit(YAML::Node const& node) -> bool
{
    if (!node.IsMap() || !node["server"]) return false;

    auto server = node["server"];

    std::string src_dir = server["src_dir"].as<std::string>();
    int port = server["port"].as<int>();
    int trigger_mode = server["trigger_mode"].as<int>();
    int timeout = server["timeout"].as<int>();
    bool opt_linger = server["opt_linger"].as<bool>();

    auto timer = Timer::ptr(new Timer());
    auto thread_pool = server["thread"].as<ThreadPool::ptr>();

    InstanceManager::AddInstance<WebServer>(
        src_dir, port, trigger_mode, timeout, opt_linger,
        std::move(timer), std::move(thread_pool));

    return true;
}

#endif // __SERVER_CONFIG__H_