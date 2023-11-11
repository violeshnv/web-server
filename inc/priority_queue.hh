#ifndef __PRIORITY_QUEUE__H_
#define __PRIORITY_QUEUE__H_

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <vector>

template <typename T, typename Less = std::less<T>>
class PriorityQueue
{
public:
    typedef PriorityQueue self;

    template <typename... Args>
    PriorityQueue(Args... args) :
        element_(std::forward<Args>(args)...) { sort(); }

    auto empty() const -> bool { return size() == 0; }
    auto size() const -> size_t { return element_.size(); }
    auto capacity() const -> size_t { return element_.capacity(); }

    void reserve(size_t n) { element_.reserve(n); }
    void clear() { element_.clear(); }

    auto cbegin() const { element_.cbegin(); }
    auto cend() const { element_.cend(); }
    auto begin() const { cbegin(); }
    auto end() const { cend(); }

    void sort()
    {
        std::ranges::sort(std::ranges::reverse_view(element_), Less());
    }

    auto operator[](size_t i) const -> T const& { return element_[i]; }

    template <typename U>
    void push(U&& value)
    {
        return emplace(std::forward<U>(value));
    }

    template <typename... Args>
    void emplace(Args&&... args)
    {
        element_.emplace_back(std::forward<Args>(args)...);
        return shift_up_(size() - 1);
    }

    auto top() const -> T const&
    {
        assert(!empty());
        return element_.front();
    }

    auto top() -> T&
    {
        return const_cast<T&>(const_cast<self const*>(this)->top());
    }

    void pop(size_t i = 0)
    {
        swap_(i, size() - 1);
        element_.pop_back();
        shift_(i);
    }

    void swap(self& other)
    {
        std::swap(element_, other.element_);
    }

private:
    auto shift_(size_t i) -> size_t
    {
        size_t r = shift_down_(i);
        if (r == i) r = shift_up_(i);
        return r;
    }

    auto shift_down_(size_t i = 0) -> size_t
    {
        size_t n = size();
        size_t down;
        while ((down = i * 2 + 1) < n) {
            if (down + 1 < n && less_(down, down + 1)) ++down;
            if (less_(down, i)) break;
            swap_(down, i);
            i = down;
        }
        return i;
    }

    auto shift_up_(size_t i) -> size_t
    {
        size_t up;
        while (i && less_(up = (i - 1) / 2, i)) {
            swap_(i, up);
            i = up;
        }
        return i;
    }

    void swap_(size_t i, size_t j)
    {
        if (i == j) return;
        std::swap(element_[i], element_[j]);
    }

    auto less_(size_t i, size_t j) const -> bool
    {
        return Less()(element_[i], element_[j]);
    }

    std::vector<T> element_;
};

template <typename Index, typename T, typename Less = std::less<T>>
class MapPriorityQueue
{
public:
    typedef MapPriorityQueue self;

    MapPriorityQueue() = default;

    auto empty() const
        -> bool { return size() == 0; }
    auto size() const
        -> size_t { return element_.size(); }
    auto capacity() const
        -> size_t { return element_.capacity(); }
    auto contains(Index const& i) const
        -> bool { return index_.contains(i); }

    void reserve(size_t n) { element_.reserve(n); }
    void clear()
    {
        index_.clear();
        element_.clear();
    }

    auto operator[](Index const& i) const
        -> T const& { return element_[index_.at(i)].second; }

    template <typename U>
    void push(Index const& i, U&& value)
    {
        emplace(i, std::forward<U>(value));
    }

    template <typename... Args>
    void emplace(Index const& i, Args&&... args)
    {
        auto [it, b] = index_.try_emplace(i, size());
        if (b) {
            element_.emplace_back(i, std::forward<Args>(args)...);
            shift_up_(size() - 1);
        } else {
            element_[it->second].second = T(std::forward<Args>(args)...);
            shift_(it->second);
        }
    }

    auto top() const -> T const&
    {
        if (empty()) {
            throw std::runtime_error("empty priority queue");
        }
        return element_.front().second;
    }

    auto top() -> T&
    {
        return const_cast<T&>(const_cast<self const*>(this)->top());
    }

    template <typename U>
    void set(Index const& i, U&& value)
    {
        assert(index_.contains(i));
        size_t idx = index_.at(i);
        element_[idx].second = std::forward<U>(value);
        shift_(idx);
    }

    void pop() { pop_(0); }

    void pop(Index const& i)
    {
        assert(!empty());
        assert(index_.contains(i));
        pop_(index_.at(i));
    }

    void swap(self& other)
    {
        std::swap(element_, other.element_);
    }

private:
    void pop_(size_t i)
    {
        swap_(i, size() - 1);
        element_.pop_back();
        shift_(i);
    }

    auto shift_(size_t i) -> size_t
    {
        size_t r = shift_down_(i);
        if (r == i) r = shift_up_(i);
        return r;
    }

    auto shift_down_(size_t i = 0) -> size_t
    {
        size_t n = size();
        size_t down;
        while ((down = i * 2 + 1) < n) {
            if (down + 1 < n && less_(down, down + 1)) ++down;
            if (less_(down, i)) break;
            swap_(down, i);
            i = down;
        }
        return i;
    }

    auto shift_up_(size_t i) -> size_t
    {
        size_t up;
        while (i && less_(up = (i - 1) / 2, i)) {
            swap_(i, up);
            i = up;
        }
        return i;
    }

    void swap_(size_t i, size_t j)
    {
        if (i == j) return;
        std::swap(element_[i], element_[j]);
        std::swap(index_.at(element_[i].first),
                  index_.at(element_[j].first));
    }

    auto less_(size_t i, size_t j) const
        -> bool { return Less()(element_[i].second, element_[j].second); }

    std::map<Index, size_t> index_;
    std::vector<std::pair<Index, T>> element_;
};

template <typename T, typename Less = std::less<T>>
class IndexPriorityQueue
{
public:
    typedef IndexPriorityQueue self;

    IndexPriorityQueue() = default;

    auto empty() const -> bool { return size() == 0; }
    auto size() const -> size_t { return element_.size(); }
    auto capacity() const -> size_t { return element_.capacity(); }

    void reserve(size_t n)
    {
        pq_.reserve(n);
        element_.reserve(n);
    }

    void clear()
    {
        pq_.clear();
        element_.clear();
    }

    auto operator[](size_t i) const -> T const&
    {
        return element_[i].second;
    }

    template <typename U>
    auto push(U&& value) -> size_t
    {
        return emplace(std::forward<U>(value));
    }

    /*
    @return use it to call set(i, ...) or [i]
    */
    template <typename... Args>
    auto emplace(Args&&... args) -> size_t
    {
        size_t n = size();
        element_.emplace_back(n, std::forward<Args>(args)...);
        pq_.emplace_back(n);
        return shift_up_(n);
    }

    auto top() const -> T const&
    {
        assert(!empty());
        return element_[pq_.front()].second;
    }

    auto top() -> T&
    {
        return const_cast<T&>(const_cast<self const*>(this)->top());
    }

    template <typename U>
    void set(size_t i, U&& value)
    {
        element_[i].second = T(std::forward<U>(value));
        shift_(element_[i].first);
    }

    void pop(size_t i = 0)
    {
        assert(!empty());
        swap_element_(pq_[i], size() - 1);
        swap_(i, size() - 1);
        pq_.pop_back();
        element_.pop_back();
        shift_(i);
    }

    void del(size_t i)
    {
        pop(element_[i].first);
    }

    void swap(self& other)
    {
        std::swap(element_, other.element_);
    }

private:
    auto shift_(size_t i) -> size_t
    {
        size_t r = shift_down_(i);
        if (r == i) r = shift_up_(i);
        return r;
    }

    auto shift_down_(size_t i = 0) -> size_t
    {
        size_t n = size();
        size_t down;
        while ((down = i * 2 + 1) < n) {
            if (down + 1 < n && less_(down, down + 1)) ++down;
            if (less_(down, i)) break;
            swap_(down, i);
            i = down;
        }
        return i;
    }

    auto shift_up_(size_t i) -> size_t
    {
        size_t up;
        while (i && less_(up = (i - 1) / 2, i)) {
            swap_(i, up);
            i = up;
        }
        return i;
    }

    void swap_(size_t i, size_t j)
    {
        if (i == j) return;
        std::swap(pq_[i], pq_[j]);
        std::swap(element_[pq_[i]].first,
                  element_[pq_[j]].first);
    }

    void swap_element_(size_t i, size_t j)
    {
        if (i == j) return;
        std::swap(element_[i], element_[j]);
        std::swap(pq_[element_[i].first], pq_[element_[j].first]);
    }

    auto less_(size_t i, size_t j) const -> bool
    {
        return Less()(element_[pq_[i]].second,
                      element_[pq_[j]].second);
    }

    std::vector<size_t> pq_;
    std::vector<std::pair<size_t, T>> element_;
};

#endif // __PRIORITY_QUEUE__H_