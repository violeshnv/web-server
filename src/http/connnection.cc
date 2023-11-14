#include "http.hh"
#include "utils.hh"

bool HttpConnection::et;
std::filesystem::path HttpConnection::base_;
std::atomic<int> HttpConnection::user_count;

HttpConnection::HttpConnection(int fd, ::sockaddr_in const& addr) :
    fd_(fd), addr_(addr)
{
    ++user_count;
    LOG_INFO("create connection " + std::to_string(fd) +
             " " + ::inet_ntoa(addr.sin_addr));
}

HttpConnection::~HttpConnection()
{
    Close();
}

auto HttpConnection::Read() -> ssize_t
{
    LOG_INFO("read from ip: " + Ip() + ':' + std::to_string(Port()));

    ssize_t total_len = 0;
    ssize_t len;
    do {
        len = gulp_.read(fd_);
        if (len < 0) return len;
        total_len += len;
    } while (len && et);

    LOG_DEBUG("read done");
    return total_len;
}

auto HttpConnection::Write() -> ssize_t
{
    LOG_INFO("write to ip: " + Ip() + ':' + std::to_string(Port()));

    ssize_t total_len = 0;
    do {
        ::iovec iov[] {
            {.iov_base = res_view_.data(),  .iov_len = res_view_.size() },
            {.iov_base = file_view_.data(), .iov_len = file_view_.size()},
        };

        ssize_t len = ::writev(fd_, iov, sizeof(iov) / sizeof(iov[0]));
        if (len < 0) return ~errno;
        if (size_t(len) > res_view_.size()) {
            len -= res_view_.size();
            res_view_ = res_view_.subspan(res_view_.size());
            file_view_ = file_view_.subspan(len);
        } else {
            res_view_ = res_view_.subspan(len);
        }
        total_len += len;
    } while (ToWriteBytes() != 0 && (et || ToWriteBytes() > SWND_SIZE));

    LOG_DEBUG("write done");
    return total_len;
}

void HttpConnection::Close()
{
    if (!closed_) {
        ::close(fd_);
        closed_ = true;
        --user_count;
    }
}

auto HttpConnection::Process() -> bool
{
    req_.Clear();
    if (gulp_.empty() && req_.Lines().empty()) return false;
    bool parse_result = gulp_.empty() ? req_.Parse()
                                      : req_.Parse(std::move(gulp_));
    if (parse_result) {
        res_.Init(base_.native(), req_.Path(),
                  HttpCode::OK, req_.IsKeepAlive());
    } else {
        res_.Init(base_.native(), req_.Path(),
                  HttpCode::Bad_Request, false);
    }

    res_.Compose();
    res_view_ = res_.Response();
    file_view_ = res_.FileSpan();

    LOG_DEBUG("res_view_.size: " + std::to_string(res_view_.size()) +
              " file_view_.size: " + std::to_string(file_view_.size()));

    return true;
}

auto HttpConnection::ToWriteBytes() -> size_t
{
    return res_view_.size() + file_view_.size();
}

auto HttpConnection::IsKeepAlive() const -> bool
{
    return req_.IsKeepAlive();
}
