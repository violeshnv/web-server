#ifndef __EPOLL__H_
#define __EPOLL__H_

#include <cassert>
#include <memory>

#include <sys/epoll.h>
#include <vector>

class Epoller
{
public:
    typedef decltype(epoll_event::events) events_t;

    explicit Epoller(size_t max_event_count = 1024) :
        epoll_(::epoll_create(max_event_count)),
        events_vec_(1024)
    {
        assert(events_vec_.size());
        assert(epoll_ >= 0);
    }

    ~Epoller() { ::close(epoll_); }

    auto AddEvent(int fd, events_t events)
        -> bool { return Helper_<EPOLL_CTL_ADD>(fd, events); }
    auto RemoveEvent(int fd)
        -> bool { return Helper_<EPOLL_CTL_DEL>(fd, events_t(0)); }
    auto ChangeEvent(int fd, events_t events)
        -> bool { return Helper_<EPOLL_CTL_MOD>(fd, events); }

    auto Wait(int timeout_ms = -1) -> int
    {
        return epoll_wait(epoll_, events_vec_.data(),
                          events_vec_.size(), timeout_ms);
    }

    auto EventFd(size_t i) const -> int
    {
        assert(0 <= i && i < events_vec_.size());
        return events_vec_[i].data.fd;
    }

    events_t GetEvents(size_t i) const
    {
        assert(0 <= i && i < events_vec_.size());
        return events_vec_[i].events;
    }

private:
    template <int OP>
    auto Helper_(int fd, events_t events) -> bool
    {
        if (fd < 0) return false;
        epoll_event ev {0};
        ev.data.fd = fd;
        ev.events = events;
        return ::epoll_ctl(epoll_, OP, fd, &ev) == 0;
    }

    int epoll_;
    std::vector<::epoll_event> events_vec_;
};

#endif // __EPOLL__H_