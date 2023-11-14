#ifndef __BUFFER__H_
#define __BUFFER__H_

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <atomic>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/unistd.h>

#include "magic_enum.hh"

class Gulp
{
    static constexpr size_t SOCKET_PACKAGE_MAX_SIZE = 65535;
    static constexpr size_t ALIGN_SIZE = 1 << 8;

public:
    typedef std::byte* iterator;
    typedef std::byte const* const_iterator;

    typedef Gulp self;

    Gulp() :
        data_(nullptr), len_(0),
        read_(0), write_(0), cow_(false) { }

    Gulp(self const&) = delete;
    auto operator=(self const&) -> self& = delete;

    Gulp(self&& other) :
        data_(other.data_), len_(other.len_)
    {
        read_.exchange(other.read_);
        write_.exchange(other.write_);
        cow_.exchange(other.cow_);
        other.reset_();
    }

    auto operator=(self&& other) -> self&
    {
        this->~Gulp();
        data_ = other.data_;
        len_ = other.len_;
        read_.exchange(other.read_);
        write_.exchange(other.write_);
        cow_.exchange(other.cow_);
        other.data_ = nullptr;
        other.len_ = 0;
        other.reset_();
        return *this;
    }

    template <typename T>
    auto operator=(std::span<T> span) -> self&
    {
        this->~Gulp();
        data_ = iterator(span.data());
        len_ = span.size_bytes();
        read_ = 0;
        write_ = len_;
        cow_ = true;
        return *this;
    }

    auto operator=(std::string_view view) -> self&
    {
        return this->operator=(std::span<const char>(view));
    }

    ~Gulp();

    auto data() { return begin_(); }
    auto size() const -> size_t { return write_ - read_; }
    auto capacity() const -> size_t { return len_ - read_; }
    auto empty() const -> bool { return size() == 0; }

    auto begin() const
    {
        return const_cast<const_iterator>(const_cast<self*>(this)->begin_());
    }
    auto end() const
    {
        return const_cast<const_iterator>(const_cast<self*>(this)->end_());
    }

    void clear() { read_ = write_ = 0; }

    void reserve(size_t sz);

    auto read(int fd) -> ssize_t;

    auto write(int fd) -> ssize_t;

    void append(std::span<std::byte> span);

    auto view() -> std::string_view { return {(char const*)begin(), size()}; }
    auto span() -> std::span<std::byte const> { return {begin(), size()}; }

private:
    auto begin_() -> iterator { return data_ + read_; }
    auto end_() -> iterator { return data_ + write_; }

    void reset_();

    iterator data_;
    size_t len_;
    std::atomic<size_t> read_;
    std::atomic<size_t> write_;
    std::atomic<bool> cow_;
};

class Slurp
{
    struct FD {
        const int fd_;
        FD(int fd) :
            fd_(fd) { assert(fd_ >= 0); }
        ~FD() { close(fd_); }
        constexpr operator int() const { return fd_; }
    };

public:
    enum class State {
        INIT = 0,
        OPEN,
        FADVISE,
        FSTATE,
        MEMALIGN,
        MADVISE,
        READ,
        FINISH
    };

    typedef Slurp self;
    Slurp() { clear_(); }

    Slurp(self const&) = delete;
    auto operator=(self const&) -> self& = delete;

    Slurp(self&& other) :
        Slurp() { *this = std::move(other); }

    auto operator=(self&& other) -> self&
    {
        this->~Slurp();
        begin_ = other.begin_;
        size_ = other.size_;
        file_stat_ = other.file_stat_;
        r_ = other.r_;
        state_ = other.state_;
        error_message_ = other.error_message_;
        other.clear_();
        return *this;
    }

    Slurp(std::string_view path);

    ~Slurp();

    auto begin() const { return begin_; }
    auto const end() const { return begin_ + size_; }
    auto size() const { return size_; }
    auto empty() const { return size() == 0; }

    auto view() const
        -> std::string_view { return {(char*)begin(), size()}; }

    auto span() const
        -> std::span<std::byte> { return {begin(), size()}; }

    auto const& file_stat() const { return file_stat_; }
    auto state() const { return state_; }
    auto state_message() const
        -> std::string_view { return magic_enum::enum_name(state_).value(); }

    auto const& error_message() const { return error_message_; }
    auto const& errer_no() const { return r_; }

private:
    void clear_();

    auto error_check(long r, State state) -> bool;

    std::byte* begin_ = nullptr;
    size_t size_ = 0;

    struct ::stat file_stat_;

    long r_;
    State state_;
    std::optional<std::string> error_message_;
};

#endif // __BUFFER__H_