#include "config/config.hh"
#include "server/server.hh"

#include "instance/instance.hh"

int main(int argc, char** argv)
{
    constexpr std::string_view default_yaml {
        "config.yaml"};

    std::string_view config_filename = argc >= 2 ? argv[1] : default_yaml;

    Config& c = Config::Instance();
    c.LoadFile(std::string(config_filename));
    c.Initialize();

    auto& server = InstanceManager::GetInstance<WebServer>();
    server.Start();
}