#ifndef __SERVER__H_
#define __SERVER__H_

#include <filesystem>
#include <string>

#include <arpa/inet.h>

#include "http/epoll.hh"
#include "http/http.hh"
#include "log/log.hh"
#include "thread/thread.hh"
#include "timer/timer.hh"

class WebServer
{
public:
    typedef WebServer self;
    typedef std::unique_ptr<self> ptr;

    WebServer(std::string_view src_dir,
              int port, int trigger_mode,
              int timeout, bool opt_linger,
              Timer::ptr&& timer, ThreadPool::ptr&& thread_pool);

    ~WebServer();

    void Start();

private:
    auto InitSocket_() -> bool;

    void InitEventMode_(int trigger_mode);

    void AddClient_(int fd, sockaddr_in const& addr);

    void DealListen_();

    void DealWrite_(HttpConnection::ptr client);

    void DealRead_(HttpConnection::ptr client);

    void SendError_(int fd, std::string_view message);

    void ExtentTime_(HttpConnection::ptr client);

    void CloseConn_(HttpConnection::ptr client);

    void OnRead_(HttpConnection::ptr client);

    void OnWrite_(HttpConnection::ptr client);

    void OnProcess(HttpConnection::ptr client);

    static constexpr int MAX_FD = 65536;

    static auto SetFdNonBlock(int fd) -> int;

    std::filesystem::path src_dir_;

    int port_;
    int timeout_;
    bool linger_;

    int listen_fd_;
    bool closed_;

    Epoller::events_t listen_event_;
    Epoller::events_t connect_event_;

    std::unique_ptr<Timer> timer_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConnection::ptr> connections_;
};

#endif // __SERVER__H_