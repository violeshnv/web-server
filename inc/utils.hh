#ifndef __UTILS__H_
#define __UTILS__H_

#include <functional>
#include <memory>
#include <optional>
#include <source_location>
#include <string>

template <typename P>
    requires requires(P p) { *p < *p; }
struct dereference_less {
    auto operator()(P const& lhs, P const& rhs)
        -> bool { return *lhs < *rhs; }
};

template <typename F, typename... Args>
__always_inline static auto invoke_throw_posix_error(F&& f, Args&&... args)
{
    auto r = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    if (r) throw std::system_error(r, std::system_category());
    // return r;
}

template <typename F, typename... Args>
__always_inline static auto invoke_throw_errno(F&& f, Args&&... args)
{
    auto r = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    if (r < 0) throw std::system_error(errno, std::system_category());
    // return r < 0 ? errno : ~r;
}

template <typename F, typename... Args>
__always_inline static auto invoke_posix_error(F&& f, Args&&... args)
{
    auto r = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    // if (r) throw std::system_error(r, std::system_category());
    return r;
}
template <typename F, typename... Args>
__always_inline static auto invoke_errno(F&& f, Args&&... args)
{
    auto r = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    // if (r < 0) throw std::system_error(errno, std::system_category());
    return r < 0 ? errno : ~r;
}

__always_inline auto error_message(int r) -> std::optional<std::string>
{
    if (r > 0) return std::system_category().message(r);
    return std::nullopt;
}

__always_inline static constexpr auto is_exp2(auto n)
    -> bool { return ((n - 1) & n) == 0; }

__always_inline static constexpr auto round_up(auto n, auto size)
{
    assert(is_exp2(size));
    return (n + size - 1) & ~(size - 1);
}

#endif // __UTILS__H_