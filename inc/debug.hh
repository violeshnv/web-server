#ifndef __DEBUG__H_
#define __DEBUG__H_

#ifndef NDEBUG

#include <cassert>
#include <cctype>
#include <climits>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include <algorithm>
#include <compare>
#include <coroutine>
#include <functional>
#include <memory>
#include <random>
#include <ranges>
#include <type_traits>

#include <array>
#include <bitset>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <string>
#include <string_view>

#include <any>
#include <optional>
#include <utility>
#include <variant>

#include <filesystem>
#include <source_location>

#include "pprint.hh"

inline namespace pprint_inline
{
template <typename... Ts>
void print(Ts&&... args)
{
    static pprint::PrettyPrinter printer;
    printer.print(std::forward<Ts>(args)...);
}
} // namespace pprint_inline

#define debug(...) \
    print("[DEBUG]", __VA_ARGS__)

#define debug_fmt(...) \
    printf("[DEBUG] " __VA_ARGS__)

#define debug_val(val) \
    print("[DEBUG VALUE] " #val " =", val)

#define debug_fn(fn, ...) \
    fn(__VA_ARGS__)

#define debug_block(...)          \
    do {                          \
        printf("[DEBUG BLOCK] "); \
        __VA_ARGS__               \
    } while (0)

#else
#define debug(...)
#define debugfmt(...)
#define debug_val(...)
#define debug_fn(...)
#define debug_block(...)

#endif

#endif // __DEBUG__H_