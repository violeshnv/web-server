#include "server.hh"

WebServer::WebServer(std::string_view src_dir,
                     int port, int trigger_mode, int timeout, bool opt_linger,
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
        closed_ = true;
        LOG_FATAL("Server Init Failed!");
    }

    LOG_INFO("Server Init Success");
}

WebServer::~WebServer()
{
    ::close(listen_fd_);
    closed_ = true;
}

void WebServer::Start()
{
    Timer::rep t = -1;
    while (!closed_) {
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
            auto conn = connections_.at(fd);

            if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                CloseConn_(conn);
            } else if (events & EPOLLIN) {
                DealRead_(conn);
            } else if (events & EPOLLOUT) {
                DealWrite_(conn);
            } else {
                LOG_ERROR("unknown event");
            }
        }
    }

    LOG_INFO("QUIT SERVER");
}

void WebServer::InitEventMode_(int trigger_mode)
{
    trigger_mode = std::min(trigger_mode, 3);
    listen_event_ = EPOLLRDHUP |
                    ((trigger_mode & 0b10) ? EPOLLET : 0);
    connect_event_ = EPOLLONESHOT | EPOLLRDHUP |
                     ((trigger_mode & 0b01) ? EPOLLET : 0);
    HttpConnection::et = connect_event_ & EPOLLET;
}

void WebServer::AddClient_(int fd, sockaddr_in const& addr)
{
    auto conn = HttpConnection::ptr(new HttpConnection {fd, addr});
    auto [it, b] = connections_.emplace(fd, std::move(conn));

    if (timeout_ > 0) {
        timer_->AddEvent(fd, timeout_,
                         std::bind(&self::CloseConn_, this, it->second));
    }
    epoller_->AddEvent(fd, connect_event_ | EPOLLIN);
    SetFdNonBlock(fd);

    if (b) LOG_INFO("add client " + std::to_string(fd));
    else LOG_INFO("re-add client " + std::to_string(fd));
}

void WebServer::DealListen_()
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

void WebServer::DealWrite_(HttpConnection::ptr client)
{
    ExtentTime_(client);
    thread_pool_->AddTask(std::bind(&self::OnWrite_, this, client));
    LOG_INFO("DealWrite " + std::to_string(client->Fd()));
}

void WebServer::DealRead_(HttpConnection::ptr client)
{
    ExtentTime_(client);
    thread_pool_->AddTask(std::bind(&self::OnRead_, this, client));
    LOG_INFO("DealRead " + std::to_string(client->Fd()));
}

void WebServer::SendError_(int fd, std::string_view message)
{
    int r = ::send(fd, message.data(), message.size(), 0);
    if (r < 0) LOG_WARN("Fail to send error to " + std::to_string(fd));
    ::close(fd);
}

void WebServer::ExtentTime_(HttpConnection::ptr client)
{
    assert(client);
    if (timeout_ > 0) timer_->AdjustEvent(client->Fd(), timeout_);
}

void WebServer::CloseConn_(HttpConnection::ptr client)
{
    assert(client);
    int fd = client->Fd();
    epoller_->RemoveEvent(fd);
    client->Close();
    // connections_.erase(fd);
    LOG_INFO("close client " + std::to_string(fd));
}

void WebServer::OnRead_(HttpConnection::ptr client)
{
    assert(client);
    int r = client->Read();
    if (r < 0 && ~r != EAGAIN) {
        CloseConn_(client);
        LOG_INFO("on read: " + error_message(~r).value());
    }
    OnProcess(client);
}

void WebServer::OnWrite_(HttpConnection::ptr client)
{
    assert(client);
    int r = client->Write();
    if (client->ToWriteBytes() == 0) {
        if (client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    } else if (r < 0 && ~r == EAGAIN) {
        epoller_->ChangeEvent(client->Fd(),
                              connect_event_ | EPOLLOUT);
        return;
    }
    CloseConn_(client);
}

void WebServer::OnProcess(HttpConnection::ptr client)
{
    assert(client);
    if (client->Process()) {
        epoller_->ChangeEvent(client->Fd(), connect_event_ | EPOLLOUT);
    } else {
        epoller_->ChangeEvent(client->Fd(), connect_event_ | EPOLLIN);
    }
}

auto WebServer::SetFdNonBlock(int fd) -> int
{
    assert(fd > 0);
    return ::fcntl(fd, F_SETFL,
                   ::fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

auto WebServer::InitSocket_() -> bool
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