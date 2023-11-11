#include "config.hh"

void Config::Initialize() const
{
    for (auto const& [n, f] : inits_) {
        if (!f(node_)) {
            throw std::runtime_error(
                "Fail to parse config \"" + n + "\"");
        }
    }
}

#include "log_config.hh"
#include "server_config.hh"

int Initialize()
{
    Config::Instance().AddInit("log", LogInit);
    Config::Instance().AddInit("server", ServerInit);
    return 0;
}

auto _ = Initialize();