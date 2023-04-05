#pragma once

#include <sco/async.hpp>

namespace sco {

// A special case of one Future.
template<typename Future>
auto all(Future&& fut) {
    // NOLINTNEXTLINE(bugprone-move-forwarding-reference)
    return std::move(fut);
}

// A special case of iterable container of Future.
template<typename Iter, std::enable_if_t<std::is_base_of_v<
    std::input_iterator_tag,
    typename std::iterator_traits<Iter>::iterator_category
>>* = nullptr>
auto all(Iter begin, Iter end) {
    using Ret = typename detail::future_return_type<typename std::iterator_traits<Iter>::value_type>::type;

    class future: protected detail::future_base {
    private:
        Iter begin_, end_;

    private:
        int pending_count() {
            return std::distance(begin_, end_);
        }

        void set_sync_object(const detail::sync_object& sync) {
            for (auto it = begin_; it != end_; ++it) {
                detail::future_caller::set_sync_object(*it, sync);
            }
        }

        void resume() {
            for (auto it = begin_; it != end_; ++it) {
                detail::future_caller::resume(*it);
            }
        }

        std::exception_ptr return_exception() {
            for (auto it = begin_; it != end_; ++it) {
                auto ex = detail::future_caller::return_exception(*it);
                if (ex) {
                    return ex;
                }
            }
            return {};
        }

        auto return_value() {
            if constexpr (!std::is_void_v<Ret>) {
                std::vector<Ret> ret;
                for (auto it = begin_; it != end_; ++it) {
                    ret.push_back(detail::future_caller::return_value(*it));
                }
                return ret;
            }
        }

        friend detail::future_caller;

    public:
        constexpr future(Iter&& begin, Iter&& end):
            begin_(std::move(begin)), end_(std::move(end)) {}
    };
    return future(std::move(begin), std::move(end));
}

namespace detail {

struct exception_first {
    std::exception_ptr ex;

    template<typename Future>
    void any(Future& fut) {
        if (ex) {
            return;
        }

        ex = detail::future_caller::return_exception(fut);
    }
};

template<typename T>
struct future_tuple_is_return_void: public std::false_type {};

template<typename... Future>
struct future_tuple_is_return_void<std::tuple<Future...>>:
    public std::conjunction<std::is_void<typename future_return_type<Future>::type>...> {};

template<typename Future>
auto future_return_tuple(Future& fut) {
    if constexpr (!std::is_void_v<typename future_return_type<Future>::type>) {
        return std::make_tuple(future_caller::return_value(fut));
    } else {
        return std::tuple<>{};
    }
}

} // namespace detail

template<typename... Future>
auto all(Future&&... futs) {
    std::tuple<Future&&...> futTuple{std::forward<Future>(futs)...};

    using FT = decltype(futTuple);

    class future: private detail::future_nocopy {
    private:
        FT ft_;

    private:
        constexpr int pending_count() const noexcept {
            return std::tuple_size_v<FT>;
        }

        void set_sync_object(const detail::sync_object& sync) {
            std::apply([&](auto&&... fut) {
                (detail::future_caller::set_sync_object(fut, sync), ...);
            }, ft_);
        }

        void resume() {
            std::apply([](auto&&... fut) {
                (detail::future_caller::resume(fut), ...);
            }, ft_);
        }

        auto return_value() {
            if constexpr (!detail::future_tuple_is_return_void<FT>::value) {
                return std::apply([](auto&&... fut) {
                    return std::tuple_cat(detail::future_return_tuple(fut)...);
                }, ft_);
            }
        }

        std::exception_ptr return_exception() {
            detail::exception_first ex;
            std::apply([&](auto&&... fut) {
                (ex.any(fut), ...);
            }, ft_);
            return ex.ex;
        }

        friend detail::future_caller;

    public:
        explicit future(FT&& ft): ft_(std::move(ft)) {}
    };
    return future(std::move(futTuple));
}

} // namespace sco
