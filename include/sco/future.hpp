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
    template<typename T>
    inline static int pending_count(T& x) {
        return x.pending_count();
    }
    template<typename T>
    inline static void set_sync_object(T& x, const sync_object& sync) {
        x.set_sync_object(sync);
    }
    template<typename T>
    inline static void resume(T& x) {
        x.resume();
    }
    template<typename T>
    inline static auto return_value(T& x) {
        return x.return_value();
    }
    template<typename T>
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
struct future_traits {
    using return_type = std::invoke_result_t<decltype(future_caller::return_value<T>), T&>;
};

} // namespace sco::detail

#ifdef SCO_HEADER_ONLY
# include <sco/future-inl.hpp>
#endif
