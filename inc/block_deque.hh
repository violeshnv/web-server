#ifndef __BLOCKQUEUE__H_
#define __BLOCKQUEUE__H_

#include <condition_variable>
#include <deque>
#include <mutex>
#include <sys/time.h>

// TODO

template <typename T>
class BlockDeque
{
public:
    explicit BlockQueue(size_t max_capacity = 1024);
    ~BlockQueue();

private:
    std::deque<T> deq_;
    size_t size_, capacity_;

    std::mutex mutex_;
    bool close_;

    std::condition_variable condConsumer_;

    std::condition_variable condProducer_;
};

#endif // __BLOCKQUEUE__H_