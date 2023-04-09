#pragma once

namespace sco::detail {

template<typename T, typename=void>
struct is_awaiter: public std::false_type {};

template<typename T>
struct is_awaiter<T, std::void_t<
    decltype(&T::await_suspend),
    decltype(std::declval<T>().await_resume()),
    std::enable_if_t<
        std::is_same_v<bool, std::invoke_result_t<decltype(&T::await_ready), T>>
>>>: public std::true_type {};

template<typename T, typename=void>
struct is_return_awaiter: public std::false_type {};

template<typename T>
struct is_return_awaiter<T, std::enable_if_t<
    is_awaiter<std::invoke_result_t<decltype(&T::operator co_await), T>>::value
>>: public std::true_type {};

// awaitable can be either an awaiter
// or a type that returns an awaiter from its operator co_await().
template<typename T>
struct is_awaitable: public std::disjunction<is_awaiter<T>, is_return_awaiter<T>> {};

template<typename T, typename=void>
struct awaitable_traits;

template<typename T>
struct awaitable_traits<T, std::enable_if_t<is_awaiter<T>::value>> {
    using awaiter_type = T;
    using return_type = std::invoke_result_t<decltype(&awaiter_type::await_resume), awaiter_type>;
};

template<typename T>
struct awaitable_traits<T, std::enable_if_t<is_return_awaiter<T>::value>> {
    using awaiter_type = std::invoke_result_t<decltype(&T::operator co_await), T>;
    using return_type = std::invoke_result_t<decltype(&awaiter_type::await_resume), awaiter_type>;
};

} // namespace sco::detail
