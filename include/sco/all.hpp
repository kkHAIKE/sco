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
template<typename Iter, typename std::enable_if<std::is_same<
    typename std::iterator_traits<Iter>::iterator_category,
    std::input_iterator_tag>::value>::type* = nullptr>
auto all(Iter begin, Iter end) {
    class future: protected detail::future_base,
        protected detail::future_with_value<void>{
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
            return std::apply([](auto&&... fut) {
                return std::make_tuple(detail::future_caller::return_value(fut)...);
            }, ft_);
        }

        std::exception_ptr return_exception() {
            detail::exception_first ex;
            std::apply([&](auto&&... fut) {
                (ex.any(fut), ...);
            }, ft_);
            return ex.ex;
        }

    public:
        explicit future(FT&& ft): ft_(std::move(ft)) {}
    };
    return future(std::move(futTuple));
}

} // namespace sco

