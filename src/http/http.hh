#ifndef __HTTP__H_
#define __HTTP__H_

#include <filesystem>
#include <functional>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <arpa/inet.h>

#include "buffer/buffer.hh"
#include "log/log.hh"
#include "magic_enum.hh"

class HttpCode
{
public:
    enum : int {
        Unknown = -1,
        OK = 200,
        Bad_Request = 400,
        Forbidden = 403,
        Not_Found = 404,
    };

private:
    typedef decltype(OK) Code;

public:
    constexpr HttpCode() :
        HttpCode(HttpCode::Unknown) { }
    constexpr HttpCode(Code code) :
        code_(code) { }
    constexpr HttpCode(int code) :
        HttpCode(static_cast<Code>(code)) { }

    operator std::string_view() const { return code_name.at(*this); }
    constexpr operator int() const { return static_cast<int>(code_); }

private:
    Code code_;

    static const std::unordered_map<int, std::string_view> code_name;
};

class HttpRequest
{

public:
    enum class ParseState {
        REQUEST_LINE = 0,
        HEADERS,
        BODY,
        FINISH,
    };

    HttpRequest() :
        state_(ParseState::REQUEST_LINE),
        method_(), version_(), body_(), path_(), header_() { }

    void Clear();

    template <typename U>
    auto Parse(U&& data) -> bool
    {
        raw_data_ = std::forward<U>(data);
        return Parse_();
    }

    auto& Path() { return path_; }
    auto const& Path() const { return path_; }
    auto const& Method() const { return method_; }
    auto const& Version() const { return version_; }
    auto const& Header() const { return header_; }
    auto const& Body() const { return body_; }

    auto IsKeepAlive() const -> bool;

private:
    auto Parse_() -> bool;

    auto SliceLines_(std::string_view view)
        -> std::vector<std::string_view>;

    auto ParseRequestLine_(std::string_view& line) -> bool;

    void ParseHeader_(std::string_view& line);

    void ParseBody_(std::string_view& line);

    void ParsePath_();

    void ParsePost_()
    {
        // TODO
    }

    Gulp raw_data_;
    ParseState state_;
    std::string_view method_, version_, body_;
    std::string path_;
    std::unordered_map<std::string_view, std::string_view> header_;

    static const std::unordered_set<std::string_view> default_html;
};

class HttpResponse
{
public:
    HttpResponse() = default;

    ~HttpResponse() = default;

    void Init(std::string_view base, std::string_view path,
              HttpCode code = HttpCode::Unknown, bool keep_alive = false);

    void Compose();

    auto& Response() { return response_; }
    auto const& Code() const { return code_; }

    int ErrorNo() const
    {
        return slurp_.error_message() ? slurp_.errer_no() : 0;
    }

    auto const& ErrorMessage() const { return slurp_.error_message(); }

    auto FileView() -> std::string_view { return slurp_.view(); }
    auto FileSpan() -> std::span<char>
    {
        return {(char*)slurp_.span().data(), slurp_.span().size()};
    }

private:
    void ComposeCode_();

    void Redirect_();

    void ComposeState_();

    void ComposeHeader_();

    void ComposeContent_();

    constexpr auto ErrorHtml_() -> std::string_view;

    auto FileType_() -> std::string_view;

    std::list<std::string_view> res_lst_;
    std::list<std::string> temp_;
    std::string response_;

    std::filesystem::path full_path_;
    Slurp slurp_;

    HttpCode code_;
    bool keep_alive_;

    static const std::unordered_map<std::string_view, std::string_view>
        suffix_type;
    static const std::unordered_map<int, std::string_view> code_path;
};

class HttpConnection
{
    static constexpr size_t SWND_SIZE = 10240;

public:
    struct FD {
        int fd_;
        constexpr FD(int fd) :
            fd_(fd) { }
        FD(FD const&) = delete;
        FD(FD&&) = delete;
        auto operator=(FD&) = delete;
        auto operator=(FD&&) = delete;
        auto operator=(int fd) -> FD&
        {
            fd_ = fd;
            return *this;
        }
        ~FD() { Close(); }
        void Close()
        {
            if (!IsClosed()) ::close(fd_);
            fd_ = -1;
        }
        auto IsClosed() -> bool const { return fd_ == -1; }
        constexpr operator int() const { return fd_; }
    };

    typedef HttpConnection self;
    typedef std::unique_ptr<self> ptr;

    HttpConnection(int fd, ::sockaddr_in const& addr);

    ~HttpConnection();

    auto Fd() const -> int const { return fd_; }
    auto const& Port() const { return addr_.sin_port; }
    auto Ip() -> std::string { return ::inet_ntoa(addr_.sin_addr); }
    auto const& Addr() const { return addr_; }

    auto Read() -> ssize_t;

    auto Write() -> ssize_t;

    void Close();

    auto Process() -> bool;

    auto ToWriteBytes() -> size_t;

    auto IsKeepAlive() const -> bool;

private:
    FD fd_;
    ::sockaddr_in addr_;

    Gulp gulp_;
    HttpRequest req_;

    std::span<char> res_view_, file_view_;
    HttpResponse res_;

public:
    static bool et;
    static std::filesystem::path base_;
    static std::atomic<int> user_count;
};

#endif // __HTTP__H_