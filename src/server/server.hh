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
              Timer::ptr&& timer, ThreadPool::ptr&& thread_pool) :
        src_dir_(src_dir),
        port_(port), timeout_(timeout), linger_(opt_linger), listen_fd_(-1),
        timer_(std::move(timer)), thread_pool_(std::move(thread_pool)),
        epoller_(new Epoller(1024)), connections_()
    {
        HttpConnection::user_count = 0;
        HttpConnection::base_ = src_dir_;

        InitEventMode_(trigger_mode);
        if (!InitSocket_()) {
            listen_fd_.Close();
            LOG_FATAL("Server Init Failed!");
        }

        LOG_INFO("Server Init Success");
    }

    ~WebServer() = default;

    void Start()
    {
        Timer::rep t = -1;
        while (!listen_fd_.IsClosed()) {
            if (timeout_ > 0) t = timer_->NextTick();

            int event_count = epoller_->Wait(t);
            if (event_count < 0) {
                LOG_ERROR("epoll error!");
                continue;
            }
            while (event_count--) {
                int fd = epoller_->EventFd(event_count);
                Epoller::events_t events = epoller_->GetEvents(event_count);
                if (fd == listen_fd_) {
                    DealListen_();
                    continue;
                }

                assert(connections_.contains(fd));
                auto& conn = connections_.at(fd);

                if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                    CloseConn_(*conn);
                } else if (events & EPOLLIN) {
                    DealRead_(*conn);
                } else if (events & EPOLLOUT) {
                    DealWrite_(*conn);
                } else {
                    LOG_ERROR("unknown event");
                }
            }
        }

        LOG_INFO("QUIT SERVER");
    }

private:
    auto InitSocket_() -> bool
    {
        int r;
        std::optional<std::string> em;
        int listen_fd = -1;

#define ERROR_CHECK(f, ...)                       \
    do {                                          \
        r = invoke_errno(f, ##__VA_ARGS__);       \
        em = error_message(r);                    \
        if (em) {                                 \
            if (listen_fd != -1) {                \
                ::close(listen_fd);               \
                listen_fd = -1;                   \
            }                                     \
            LOG_FATAL(#f " fail: " + em.value()); \
            return false;                         \
        }                                         \
    } while (0)

        if (port_ < 1024 || port_ > 65535) {
            LOG_ERROR("Port: " + std::to_string(port_));
            return false;
        }

        ::sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = inet_addr("0.0.0.0");

        ERROR_CHECK(::socket, AF_INET, SOCK_STREAM, 0);
        listen_fd = ~r;

        ::linger lg {
            .l_onoff = linger_ ? 1 : 0,
            .l_linger = linger_ ? 1 : 0,
        };
        ERROR_CHECK(::setsockopt, listen_fd, SOL_SOCKET, SO_LINGER,
                    &lg, sizeof(lg));

        int optval = 1;
        ERROR_CHECK(::setsockopt, listen_fd, SOL_SOCKET, SO_REUSEADDR,
                    &optval, sizeof(optval));

        ERROR_CHECK(::bind, listen_fd, (struct sockaddr*)&addr, sizeof(addr));

        ERROR_CHECK(::listen, listen_fd, 8);

        r = epoller_->AddEvent(listen_fd, listen_event_ | EPOLLIN);
        if (!r) {
            close(listen_fd);
            LOG_FATAL("AddEvent fail:");
            return false;
        }

        SetFdNonBlock(listen_fd);

        listen_fd_ = listen_fd;

        LOG_INFO("listen socket " + std::to_string(listen_fd_) +
                 " in " + std::to_string(port_));

        return true;

#undef ERROR_CHECK
    }

    void InitEventMode_(int trigger_mode)
    {
        trigger_mode = std::min(trigger_mode, 3);
        listen_event_ = EPOLLRDHUP |
                        ((trigger_mode & 0b10) ? EPOLLET : 0);
        connect_event_ = EPOLLONESHOT | EPOLLRDHUP |
                         ((trigger_mode & 0b01) ? EPOLLET : 0);
        HttpConnection::et = connect_event_ & EPOLLET;
    }

    void AddClient_(int fd, sockaddr_in const& addr)
    {
        connections_.emplace(fd, new HttpConnection {fd, addr});

        if (timeout_ > 0) {
            timer_->AddEvent(fd, timeout_,
                             std::bind(&self::CloseConn_, this,
                                       std::ref(*connections_.at(fd))));
        }
        epoller_->AddEvent(fd, connect_event_ | EPOLLIN);
        SetFdNonBlock(fd);
        LOG_INFO("add client " + std::to_string(fd));
    }

    void DealListen_()
    {
        ::sockaddr_in addr;
        ::socklen_t len = sizeof(addr);
        do {
            int fd = ::accept(listen_fd_, (::sockaddr*)&addr, &len);
            if (fd <= 0) return;
            else if (HttpConnection::user_count >= MAX_FD) {
                SendError_(fd, "Server Busy!");
                LOG_WARN("Server Busy!");
                return;
            }
            AddClient_(fd, addr);
        } while (listen_event_ & EPOLLET);
    }

    void DealWrite_(HttpConnection& client)
    {
        ExtentTime_(client);
        thread_pool_->AddTask(
            std::bind(
                &self::OnWrite_, this, std::ref(client)));
    }
    void DealRead_(HttpConnection& client)
    {
        ExtentTime_(client);
        thread_pool_->AddTask(
            std::bind(
                &self::OnRead_, this, std::ref(client)));
    }

    void SendError_(int fd, std::string_view message)
    {
        int r = ::send(fd, message.data(), message.size(), 0);
        if (r < 0) LOG_WARN("Fail to send error to " + std::to_string(fd));
        close(fd);
    }

    void ExtentTime_(HttpConnection& client)
    {
        if (timeout_ > 0) timer_->AdjustEvent(client.Fd(), timeout_);
    }

    void CloseConn_(HttpConnection& client)
    {
        int fd = client.Fd();
        epoller_->RemoveEvent(fd);
        client.Close();
        LOG_INFO("close client " + std::to_string(fd));
    }

    void OnRead_(HttpConnection& client)
    {
        int r = client.Read();
        if (r < 0 && ~r != EAGAIN) {
            CloseConn_(client);
            LOG_INFO("on read: " + error_message(~r).value());
        }
        OnProcess(client);
    }
    void OnWrite_(HttpConnection& client)
    {
        int r = client.Write();
        if (client.ToWriteBytes() == 0) {
            if (client.IsKeepAlive()) {
                OnProcess(client);
                return;
            }
        } else if (r < 0 && ~r == EAGAIN) {
            epoller_->ChangeEvent(client.Fd(),
                                  connect_event_ | EPOLLOUT);
            return;
        }
    }

    void OnProcess(HttpConnection& client)
    {
        if (client.Process()) {
            epoller_->ChangeEvent(client.Fd(), connect_event_ | EPOLLOUT);
        } else {
            epoller_->ChangeEvent(client.Fd(), connect_event_ | EPOLLIN);
        }
    }

    static constexpr int MAX_FD = 65536;

    static int SetFdNonBlock(int fd)
    {
        assert(fd > 0);
        return ::fcntl(fd, F_SETFL,
                       ::fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    }

    std::filesystem::path src_dir_;

    int port_;
    int timeout_;
    bool linger_;

    HttpConnection::FD listen_fd_;

    Epoller::events_t listen_event_;
    Epoller::events_t connect_event_;

    std::unique_ptr<Timer> timer_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConnection::ptr> connections_;
};

#endif // __SERVER__H_