#include <iostream>

#include "config/config.hh"
#include "log/log.hh"

int main()
{
    Config& c = Config::Instance();
    c.LoadFile("test.yaml");
    c.Initialize();

    std::cout << c.Node() << '\n';

    LOG_INFO("test content");
}