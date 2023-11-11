#ifndef __INSTANCE_MANAGER__H_
#define __INSTANCE_MANAGER__H_

#include <cassert>

#include <any>
#include <memory>
#include <source_location>
#include <string_view>
#include <unordered_map>

template <typename T>
concept is_not_ref_or_const = !std::is_reference_v<T> && !std::is_const_v<T>;

class InstanceManager
{
    friend int Initialize();

    template <typename T>
    using ptr = std::shared_ptr<T>;

public:
    template <typename T>
        requires is_not_ref_or_const<T>
    static auto GetInstance() -> T&
    {
        assert(map.contains(name_detail<T>()));
        return *std::any_cast<ptr<T>&>(map.at(name_detail<T>()));
    }

    template <typename T, typename... Args>
        requires is_not_ref_or_const<T>
    static void AddInstance(Args&&... args)
    {
        map[name_detail<T>()] =
            std::make_shared<T>(std::forward<Args>(args)...);
    }

private:
    template <typename T>
    static constexpr auto name_detail() -> std::string_view
    {
        return std::source_location::current().function_name();
    }

    static std::unordered_map<std::string_view, std::any> map;
};

#endif // __INSTANCE_MANAGER__H_