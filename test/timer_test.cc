#include <iostream>
#include <thread>

#include "timer/timer.hh"

void foo(int i)
{
    std::cout << '[' << "foo " << i << ']' << '\n';
}

int main()
{
    using std::chrono_literals::operator""ms;

    Timer timer;
    timer.AddEvent(2, 5000, []() { foo(1); });
    timer.AddEvent(0, 10000, []() { foo(2); });
    timer.AddEvent(3, 1000, []() { foo(3); });

    while (!timer.Empty()) {
        std::cout << "event: " << timer.Size()
                  << " next tick: " << timer.NextTick() << '\n';
        std::this_thread::sleep_for(500ms);
    }
}