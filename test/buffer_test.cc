#include "buffer/buffer.hh"

#include <fcntl.h>
#include <iostream>

int main()
{
    Gulp g;

    int t = open("config_test.cc", O_RDONLY);
    g.read(t);
    std::cout << g.view() << '\n';

    Slurp s {"buffer_test.cc"};
    std::cout << s.view() << '\n';

    s = Slurp {".?.?."};
    if (s.error_message()) {
        std::cout << s.state_message() << ": "
                  << s.error_message().value() << '\n';
    }
}