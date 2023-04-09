#pragma once

#include <sco/promise.hpp>

// The FutureLike type that integrates with co_await must have the following functions.
// int pending_count()
// void set_sync_object(const sync_object& sync)
// void resume()
// Ret return_value()
// std::exception_ptr return_exception()

namespace sco::detail {

// The following Future-associated classes can be combined arbitrarily.

struct future_nocopy {
    future_nocopy() = default;
    ~future_nocopy() = default;
    future_nocopy(future_nocopy&) = delete;
    future_nocopy& operator=(const future_nocopy&) = delete;
    future_nocopy(future_nocopy&&) = default;
    future_nocopy& operator=(future_nocopy&&) = delete;
};

struct future_base: private future_nocopy {
    constexpr int pending_count() const noexcept { return 1; }

    std::exception_ptr exception_;
    std::exception_ptr return_exception();
};

struct future_with_sync {
    sync_object sync_;

    void set_sync_object(const sync_object& sync);
};

template<typename Ret>
struct future_with_value {
    std::optional<Ret> value_;

    Ret return_value() { return std::move(*value_); }
};

template<>
struct future_with_value<void> {
    constexpr void return_value() const noexcept {}
};

// private member function call mediator
struct future_caller {
    template<typename T, typename=std::void_t<decltype(&T::pending_count)>>
    inline static int pending_count(T& x) {
        return x.pending_count();
    }
    template<typename T, typename=std::void_t<decltype(&T::set_sync_object)>>
    inline static void set_sync_object(T& x, const sync_object& sync) {
        x.set_sync_object(sync);
    }
    template<typename T, typename=std::void_t<decltype(&T::resume)>>
    inline static void resume(T& x) {
        x.resume();
    }
    template<typename T, typename=std::void_t<decltype(&T::return_value)>>
    inline static auto return_value(T& x) {
        return x.return_value();
    }
    template<typename T, typename=std::void_t<decltype(&T::return_exception)>>
    inline static std::exception_ptr return_exception(T& x) {
        return x.return_exception();
    }
};

template<typename T, typename=void>
struct is_future: public std::false_type {};

template<typename T>
struct is_future<T, std::void_t<
    decltype(future_caller::pending_count(std::declval<T&>())),
    decltype(future_caller::set_sync_object(std::declval<T&>(), std::declval<const sync_object&>())),
    decltype(future_caller::resume(std::declval<T&>())),
    decltype(future_caller::return_value(std::declval<T&>())),
    decltype(future_caller::return_exception(std::declval<T&>()))
>>: public std::true_type {};

template<typename T>
constexpr bool is_future_v = is_future<std::decay_t<T>>::value;

template<typename T, typename=void>
struct future_traits;

template<typename T>
struct future_traits<T, std::enable_if_t<is_future_v<T>>> {
    using return_type = decltype(future_caller::return_value(std::declval<T&>()));
};

} // namespace sco::detail

#ifdef SCO_HEADER_ONLY
# include <sco/future-inl.hpp>
#endif
