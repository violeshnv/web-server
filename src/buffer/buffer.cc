#include "buffer.hh"

#include "utils.hh"

Gulp::~Gulp()
{
    if (cow_) return;
    if (data_) ::free(data_);
    data_ = nullptr;
    len_ = read_ = write_ = 0;
}

void Gulp::reserve(size_t sz)
{
    if (sz <= capacity()) return;
    sz = round_up(sz, ALIGN_SIZE);

    void* p;
    invoke_throw_posix_error(::posix_memalign, &p, ALIGN_SIZE, sz);
    std::memcpy(p, begin(), size());
    data_ = (std::byte*)p;
    len_ = sz;
    write_ -= read_;
    read_ = 0;
    cow_ = false;
}

auto Gulp::read(int fd) -> ssize_t
{
    reserve(size() + 1);
    std::byte buf[SOCKET_PACKAGE_MAX_SIZE - ALIGN_SIZE];

    size_t vacant = capacity() - size();
    ::iovec iov[2] {
        {.iov_base = end_(), .iov_len = vacant     },
        {.iov_base = buf,    .iov_len = sizeof(buf)},
    };
    ssize_t len = ::readv(fd, iov, sizeof(iov) / sizeof(iov[0]));
    if (len < 0) return ~errno;
    if (size_t(len) <= vacant) write_ += len;
    else {
        write_ += vacant;
        append({buf, len - vacant});
    }
    return len;
}

auto Gulp::write(int fd) -> ssize_t
{
    ssize_t len = ::write(fd, begin(), size());
    if (len < 0) return ~errno;
    read_ += len;
    return len;
}

void Gulp::append(std::span<std::byte> span)
{
    assert(span.data() && !span.empty());
    reserve(size() + span.size());
    std::memcpy(end_(), span.data(), span.size());
    write_ += span.size();
}

Slurp::Slurp(std::string_view path) :
    Slurp()
{
    assert(*path.end() == '\0');
    if (error_check(invoke_errno(open, path.data(), O_RDONLY),
                    State::OPEN))
        return;

    FD fd {(int)r_};

    if (error_check(invoke_posix_error(::posix_fadvise, fd,
                                       0, 0, POSIX_FADV_SEQUENTIAL),
                    State::FADVISE))
        return;

    if (error_check(invoke_errno(::fstat, fd, &file_stat_),
                    State::FSTATE))
        return;

    size_t fsize = size_t(file_stat_.st_size);
    size_t blksize = size_t(file_stat_.st_blksize);
    if (!is_exp2(blksize)) {
        error_message_ = "invalid block size" + std::to_string(blksize);
        return;
    }

    if (error_check(invoke_posix_error(::posix_memalign, (void**)&begin_, blksize, fsize),
                    State::MEMALIGN))
        return;
    size_ = fsize;

    if (error_check(invoke_posix_error(::posix_madvise, begin_, size_, POSIX_MADV_SEQUENTIAL),
                    State::MADVISE))
        return;

    if (error_check(invoke_errno(::read, fd, begin_, size_),
                    State::READ))
        return;

    state_ = State::FINISH;
}

Slurp::~Slurp()
{
    if (begin_) ::free(begin_);
    begin_ = nullptr;
    size_ = 0;
}

void Slurp::clear_()
{
    begin_ = nullptr;
    size_ = 0;
    file_stat_ = {0};
    r_ = 0;
    state_ = State::INIT;
    error_message_ = std::nullopt;
}

auto Slurp::error_check(long r, State state) -> bool
{
    state_ = state;
    if (r > 0) {
        r_ = r;
        error_message_ = std::system_category().message(r);
        return true;
    }
    r_ = ~r;
    return false;
}
