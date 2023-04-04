#pragma once

#include <tuple>

namespace sco {
namespace detail {

// An enumeration class used to describe modes of assignments.
enum class wrapper_mode {
    move, // T& a = std::move(b)
    unptr, // T& a = *b
    // ptr, // T* a = &b
};

// A wrapper that describes how assignments are made.
template<typename T, wrapper_mode m>
struct wrapper {
    using type = T;
    static const auto mode = m;
    T& value;
    constexpr explicit wrapper(T& v): value(v) {}
};

////////////

// Checks whether T is a wrapper type

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

// template<typename T>
// struct is_ptr_wrapper: public std::false_type {};

// template<typename T>
// struct is_ptr_wrapper<wrapper<T, wrapper_mode::ptr>>: public std::true_type {};

////////////

// assignment operations

template<typename Dst, typename Src,
    std::enable_if_t<!is_wrapper<std::remove_cv_t<Dst>>::value>* = nullptr>
void assign(Dst& dst, Src&& src) { dst = std::forward<Src>(src); }

template<typename Dst, typename Src,
    std::enable_if_t<is_move_wrapper<Dst>::value>* = nullptr>
void assign(const Dst& dst, Src&& src) { dst.value = std::move(src); } // NOLINT(bugprone-move-forwarding-reference)

template<typename Dst, typename Src,
    std::enable_if_t<is_unptr_wrapper<Dst>::value>* = nullptr>
void assign(const Dst& dst, Src&& src) { if (src) { dst.value = *src; } }

// template<typename Dst, typename Src,
//     std::enable_if_t<is_ptr_wrapper<Dst>::value>* = nullptr>
// void assign(const Dst& dst, Src&& src) { dst.value = &src; }

template<typename T0, typename T1, std::size_t... I>
void assign_mutl(T0& t0, T1& t1, std::index_sequence<I...>) {
    (assign(std::get<I>(t0), std::get<I>(t1)), ...);
}

} // namespace detail

// Need to use move assignment instead of regular assignment.
// T& a = std::move(b)
template<typename T>
constexpr auto wmove(T& v) { return detail::wrapper<T, detail::wrapper_mode::move>(v); }

// Need to use dereference assignment instead of regular assignment.
// T& a = *b
template<typename T>
constexpr auto wunptr(T& v) { return detail::wrapper<T, detail::wrapper_mode::unptr>(v); }

// Need to use address-of assignment instead of regular assignment.
// T* a = &b
// template<typename T>
// constexpr auto wptr(T& v) { return detail::wrapper<T, detail::wrapper_mode::ptr>(v); }

} // namespace sco
