#ifndef __TIMER__H_
#define __TIMER__H_

#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <unordered_map>

#include "priority_queue.hh"
#include "utils.hh"

class Timer
{
    typedef std::chrono::high_resolution_clock Clock;
    typedef Clock::time_point time_point_t;
    typedef std::function<void()> callback_fn;
    typedef std::chrono::milliseconds ms_t;

    class TimerEvent
    {
        friend class Timer;

    public:
        typedef TimerEvent self;
        typedef std::unique_ptr<self> ptr;

        TimerEvent(ms_t ms, callback_fn callback) :
            expire_(Clock::now() + ms), callback_(callback) { }

        void Evoke() { callback_(); }
        auto Ready() const
            -> bool { return expire_ <= Clock::now(); }
        auto LeftMS() const
            -> ms_t::rep { return (expire_ - Clock::now()).count(); }

        void SetExpire(ms_t ms) { expire_ = Clock::now() + ms; }

        auto operator<(TimerEvent const& other) const
            -> bool { return expire_ > other.expire_; }

    private:
        time_point_t expire_;
        callback_fn callback_;
    };

public:
    typedef ms_t::rep rep;

    typedef Timer self;
    typedef std::unique_ptr<self> ptr;

    Timer(size_t n = 32) :
        pq_() { pq_.reserve(n); }

    auto Empty() const -> bool { return Size() == 0; }
    auto Size() const -> size_t { return pq_.size(); }
    auto Contains(int id) const -> bool { return pq_.contains(id); }
    void Clear() { pq_.clear(); }

    auto Now() const { return Clock::now(); }

    void AddEvent(int id, rep timeout, callback_fn const& callback)
    {
        TimerEvent::ptr e {new TimerEvent(ms_t(timeout), callback)};
        pq_.emplace(id, std::move(e));
    }

    void AdjustEvent(int id, rep timeout)
    {
        auto f = pq_[id]->callback_;
        AddEvent(id, timeout, f);
    }

    void PopEvent(int id)
    {
        assert(pq_.contains(id));
        pq_.pop(id);
    }

    void EvokeEvent(int id)
    {
        assert(pq_.contains(id));
        pq_[id]->Evoke();
    }

    void Tick()
    {
        while (!Empty()) {
            auto& e = *pq_.top();
            if (!e.Ready()) break;
            e.Evoke();
            pq_.pop();
        }
    }

    auto NextTick() -> rep
    {
        Tick();
        if (Empty()) return -1;
        return std::max(rep(0), pq_.top()->LeftMS());
    }

private:
    MapPriorityQueue<int, TimerEvent::ptr,
                     dereference_less<TimerEvent::ptr>>
        pq_;
};

#endif // __TIMER__H_