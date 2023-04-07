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

template<typename T>
struct is_unique_ptr: public std::false_type {};

template<typename T>
struct is_unique_ptr<std::unique_ptr<T>>: public std::true_type {};

// private member function call mediator
struct future_caller {
    template<typename T>
    inline static int pending_count(T& x) {
        if constexpr (!is_unique_ptr<T>::value) {
            return x.pending_count();
        } else {
            return x->pending_count();
        }
    }
    template<typename T>
    inline static void set_sync_object(T& x, const sync_object& sync) {
        if constexpr (!is_unique_ptr<T>::value) {
            x.set_sync_object(sync);
        } else {
            x->set_sync_object(sync);
        }
    }
    template<typename T>
    inline static void resume(T& x) {
        if constexpr (!is_unique_ptr<T>::value) {
            x.resume();
        } else {
            x->resume();
        }
    }
    template<typename T>
    inline static auto return_value(T& x) {
        if constexpr (!is_unique_ptr<T>::value) {
            return x.return_value();
        } else {
            return x->return_value();
        }
    }
    template<typename T>
    inline static std::exception_ptr return_exception(T& x) {
        if constexpr (!is_unique_ptr<T>::value) {
            return x.return_exception();
        } else {
            return x->return_exception();
        }
    }
};

// get return type of future.
template<typename T>
struct future_return_type {
    using type = std::invoke_result_t<decltype(future_caller::return_value<T>), T&>;
};

template<typename T>
struct future_return_type<std::unique_ptr<T>> {
    using type = std::invoke_result_t<decltype(future_caller::return_value<T>), T&>;
};

} // namespace sco::detail

#ifdef SCO_HEADER_ONLY
# include <sco/future-inl.hpp>
#endif
