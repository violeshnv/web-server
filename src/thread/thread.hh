#ifndef __THREAD__H_
#define __THREAD__H_

#include <cassert>

#include <queue>

#include <memory>

#include <condition_variable>
#include <mutex>
#include <thread>

#include <functional>

class ThreadPool
{
    struct Pool {
        typedef Pool self;
        typedef std::shared_ptr<self> ptr;

        std::queue<std::function<void()>> tasks_;
        std::mutex mtx_;
        std::condition_variable cond_;
        bool closed_;
    };

public:
    typedef ThreadPool self;
    typedef std::unique_ptr<self> ptr;

    explicit ThreadPool(size_t count = 8) :
        count_(count), pool_(new Pool())
    {
        assert(count);
        while (count--) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> locker(pool->mtx_);
                while (!pool->closed_) {
                    if (!pool->tasks_.empty()) {
                        auto task {std::move(pool->tasks_.front())};
                        pool->tasks_.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    } else pool->cond_.wait(locker);
                }
            }).detach();
        }
    }

    ~ThreadPool()
    {
        if (pool_) {
            std::lock_guard<std::mutex> locker(pool_->mtx_);
            pool_->closed_ = true;
            pool_->cond_.notify_all();
        }
    }

    template <typename F>
        requires requires(F task) { task(); }
    void AddTask(F&& task)
    {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx_);
            pool_->tasks_.emplace(std::forward<F>(task));
        }
        pool_->cond_.notify_one();
    }

    auto Count() const { return count_; }

private:
    size_t count_;
    Pool::ptr pool_;
};

#endif // __THREAD__H_