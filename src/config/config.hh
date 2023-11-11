#ifndef __CONFIG__H_
#define __CONFIG__H_

#include <functional>
#include <queue>
#include <string>
#include <string_view>

#include <yaml-cpp/yaml.h>

#include "instance/instance.hh"

class Config
{
public:
    auto const& Node() const { return node_; }

    void LoadFile(std::string const& filename)
    {
        node_ = YAML::LoadFile(filename);
    }

    template <typename Callable>
    void AddInit(std::string_view name, Callable&& f)
    {
        inits_.emplace_back(name, std::forward<Callable>(f));
    };

    void Initialize() const;

    Config() = default;
    Config(Config const&) = delete;

    static auto Instance()
        -> Config&
    {
        static Config config;
        return config;
    }

private:
    YAML::Node node_;
    std::vector<
        std::pair<std::string, std::function<bool(YAML::Node)>>>
        inits_;
};

#endif // __CONFIG__H_