#pragma once

#include <type_traits>
#include <utility>

namespace sco {
namespace detail {

// 默认的赋值规则就是 T& a = b
// 使用各种包装器更改规则

enum class wrapper_mode {
    move, // 移动包装器 T& a = std::move(b)
    unptr, // 解脂针包装器 T& a = *b
    ptr, // 脂针包装器 T* a = &b
};

template<typename T, wrapper_mode m> // 参数是非引用类型 const校验?
struct wrapper {
    using type = T;
    static const auto mode = m;
    T& value;
    constexpr explicit wrapper(T& v): value(v) {}
};

template<typename T>
struct is_wrapper: public std::false_type {};

template<typename T, wrapper_mode m>
struct is_wrapper<wrapper<T, m>>: public std::true_type {};

template<typename T>
struct is_move_wrapper: public std::false_type {};

template<typename T>
struct is_move_wrapper<wrapper<T, wrapper_mode::move>>: public std::true_type {};

template<typename T>
struct is_unptr_wrapper: public std::false_type {};

template<typename T>
struct is_unptr_wrapper<wrapper<T, wrapper_mode::unptr>>: public std::true_type {};

template<typename T>
struct is_ptr_wrapper: public std::false_type {};

template<typename T>
struct is_ptr_wrapper<wrapper<T, wrapper_mode::ptr>>: public std::true_type {};

////////////

// 自适应赋值
// wrapper 会当作左值引用传递
template<typename Dst, typename Src,
    std::enable_if_t<!is_wrapper<std::remove_cv_t<Dst>>::value>* = nullptr>
void assign(Dst& dst, Src&& src) { dst = std::forward<Src>(src); }

template<typename Dst, typename Src,
    std::enable_if_t<is_move_wrapper<Dst>::value>* = nullptr>
void assign(const Dst& dst, Src&& src) { dst.value = std::move(src); }

template<typename Dst, typename Src,
    std::enable_if_t<is_unptr_wrapper<Dst>::value>* = nullptr>
void assign(const Dst& dst, Src&& src) { if (src) { dst.value = *src; } }

template<typename Dst, typename Src,
    std::enable_if_t<is_ptr_wrapper<Dst>::value>* = nullptr>
void assign(const Dst& dst, Src&& src) { dst.value = &src; }

template<typename T0, typename T1, std::size_t... I>
void assign_mutl(T0& t0, T1& t1, std::index_sequence<I...>) {
    (assign(std::get<I>(t0), std::get<I>(t1)), ...);
}

}

template<typename T>
constexpr auto wmove(T& v) { return detail::wrapper<T, detail::wrapper_mode::move>(v); }

template<typename T>
constexpr auto wunptr(T& v) { return detail::wrapper<T, detail::wrapper_mode::unptr>(v); }

template<typename T>
constexpr auto wptr(T& v) { return detail::wrapper<T, detail::wrapper_mode::ptr>(v); }

}
