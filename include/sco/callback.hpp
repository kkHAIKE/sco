#pragma once

#include <sco/promise.hpp>
#include <sco/assign.hpp>

namespace sco {
namespace detail {

struct callback_base {
    promise_shared::ptr promise;

    // resume the coroutine in the callback thread.
    void resume();
};

template<typename, typename, typename=void>
struct callback_tie;

template<typename... Args, typename Refs>
struct callback_tie<void(Args...), Refs,
    std::enable_if_t<std::tuple_size_v<Refs> == sizeof...(Args)>
>: public callback_base {
    Refs refs_;

    constexpr explicit callback_tie(Refs&& refs): refs_(std::move(refs)) {}

    void operator()(Args... args) {
        // convert the callback arguments to a reference tuple.
        std::tuple<std::add_lvalue_reference_t<Args>...> argsTuple{args...};

        // assign the callback arguments to the coroutine variables.
        assign_mutl(refs_, argsTuple, std::make_index_sequence<sizeof...(args)>());

        resume();
    }
};

// Specialization for void() callback.
template<>
struct callback_tie<void(), std::tuple<>>: public callback_base {
    constexpr explicit callback_tie(std::tuple<>&&) {}

    void operator()() {
        resume();
    }
};

// Filtered out callback_base using tuple_cat in combination.
template<typename T>
constexpr auto get_callback_base(T&& v) {
    if constexpr (std::is_base_of_v<callback_base, std::remove_cvref_t<T>>) {
        static_assert(!std::is_reference_v<T>, "callback parameter must be rvalue ref");
        return std::tuple<callback_base&>{v};
    } else {
        return std::tuple<>{};
    }
}

} // namespace detail

// create a callback function like std::tie to capture the callback arguments.
template<typename Sign, typename... Refs>
constexpr auto cb_tie(Refs&&... refs) {
    // Requires copying of the rvalue reference.
    return detail::callback_tie<Sign, std::tuple<Refs...>>({std::forward<Refs>(refs)...});
}

// Wrap an asynchronous function into a Future for use with co_await.
template<typename F, typename... Args>
auto call_with_callback(F&& f, Args&&... args) {
    // collect all callback_base
    auto cbs = std::tuple_cat(detail::get_callback_base(std::forward<Args>(args))...);
    // only support one callback
    static_assert(std::tuple_size_v<decltype(cbs)> > 0, "call_with_callback must be call with a callback");

    auto& cb = std::get<0>(cbs);
    std::tuple<Args&&...> argsTuple{std::forward<Args>(args)...};

    using CB = decltype(cb);
    using AT = decltype(argsTuple);

    class future: protected detail::future_base,
        protected detail::future_with_value<void> {
    private:
        CB cb_;
        AT at_;
        F&& f_;

    private:
        void set_sync_object(const detail::sync_object& sync) {
            cb_.promise = sync;
        }

        void resume() {
            // call the original function.
            try {
                std::apply(std::forward<F>(f_), std::move(at_));
            } catch (...) {
                exception_ = std::current_exception();
            }
        }

        friend detail::future_caller;

    public:
        future(CB cb, AT&& at, F&& f)
            : cb_(cb), at_(std::move(at)), f_(std::forward<F>(f)) {}

        // no-copytable
        future(future&) = delete;
        future& operator=(const future&) = delete;
        future(future&&) = default;
    };

    return future(cb, std::move(argsTuple), std::forward<F>(f));
}

} // namespace sco

#ifdef SCO_HEADER_ONLY
# include <sco/callback-inl.hpp>
#endif
