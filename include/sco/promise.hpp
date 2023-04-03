#pragma once

#include <sco/common.h>

#include <memory>
#include <atomic>
#include <optional>

namespace sco {
namespace detail {

// forward declaration
struct promise_type_base;

// Using reference counting ensures that the current thread
// can operate on the coroutine.
struct promise_shared {
    using ptr = std::shared_ptr<promise_shared>;

    // When the counter reaches 0, it means that the current thread
    // is able to operate on the coroutine, such as resuming it.
    std::atomic_int await_pending;
    bool release_and_check_await_done();

    promise_type_base* promise{};

    // underlying address of the coroutine_handle
    void *handle_address{};
    COSTD::coroutine_handle<> handle();

    promise_shared(int pending, promise_type_base* promise, COSTD::coroutine_handle<> h);
};

// Used in a thread context to obtain additional results
// after the root coroutine is finished.
struct root_result {
    using opt = std::optional<root_result>;

    std::exception_ptr exception;

    // underlying address of the coroutine_handle
    void *root_handle_address{};
    COSTD::coroutine_handle<> root_handle();
};

// The synchronization object passed between Awaiter and Future
// may have additional supplements in the future.
using sync_object = promise_shared::ptr;
sync_object make_sync_object(int pending, promise_type_base& promise, COSTD::coroutine_handle<> h);

} // namespace detail
} // namespace sco

// require sync_object defined.
#include <sco/future.hpp>

namespace sco {
namespace detail {

// Basic implementation of coroutine Promise
struct promise_type_base {
    // coroutines should start executing when co_awaited or resumed explicitly."
    constexpr COSTD::suspend_always initial_suspend() const noexcept { return {}; }

    // Pointer points to thread stack, allowing thread to obtain root coroutine finish status.
    root_result::opt* root_{};
    constexpr void set_root_result_from_thread(root_result::opt& res) { root_ = &res; }

    // The awaiter returned by final_suspend.
    struct final_awaiter {
        constexpr bool await_ready() const noexcept { return false; }

        // Returning to the parent coroutine handle can automatically resume the parent coroutine.
        COSTD::coroutine_handle<> await_suspend_(COSTD::coroutine_handle<> h, promise_type_base& promise);

        // By instantiating the Promise class with a template, the promise function can be called.
        template<typename Child>
        COSTD::coroutine_handle<> await_suspend(COSTD::coroutine_handle<Child> h) noexcept {
            return await_suspend_(h, h.promise());
        }

        // The final_suspend awaiter ignores the return value.
        constexpr void await_resume() const noexcept {}
    };
    constexpr final_awaiter final_suspend() const noexcept { return {}; }

    std::exception_ptr exception_;
    // Capturing unhandled exceptions in the current coroutine.
    void unhandled_exception();

    // Save the synchronization object passed by the Awaiter.
    sync_object sync_;

    // This awaiter connects co_await with the Future.
    template<typename Future>
    struct future_awaiter {
        using Ret = typename future_return_type<Future>::type;
        Future fut;

        constexpr bool await_ready() const noexcept { return false; }

        template<typename Child>
        bool await_suspend(COSTD::coroutine_handle<Child> h) {
            auto sync = make_sync_object(future_caller::pending_count(fut) + 1, h.promise(), h);
            future_caller::set_sync_object(fut, sync);
            future_caller::resume(fut);

            // If false is returned, then resume the coroutine.
            return !sync->release_and_check_await_done();
        }

        Ret await_resume()  {
            auto ex = future_caller::return_exception(fut);
            if (ex) {
                std::rethrow_exception(ex);
            }
            return future_caller::return_value(fut);
        }
    };

    template<typename Future>
    auto await_transform(Future&& fut) {
        return future_awaiter<std::remove_reference_t<Future>>{std::move(fut)}; // NOLINT(bugprone-move-forwarding-reference)
    }
};

// Promise type use with coroutine_handle.
template<typename Coro, typename Ret>
struct promise_type: public promise_type_base {
    using handle_type = COSTD::coroutine_handle<promise_type>;

    // make the coroutine_handle from this promise.
    auto get_return_object() {
        return Coro(handle_type::from_promise(*this));
    }

    std::optional<Ret> value_;

    // return value via co_return.
    template<typename T, typename = std::enable_if_t<std::is_convertible_v<Ret, T>>>
    void return_value(T&& v) noexcept {
        value_ = std::forward<T>(v);
    }
};

// Specialization for void return type.
template<typename Coro>
struct promise_type<Coro, void>: public promise_type_base {
    using handle_type = COSTD::coroutine_handle<promise_type>;

    auto get_return_object() {
        return Coro(handle_type::from_promise(*this));
    }

    constexpr void return_void() const noexcept {}
};

} // namespace detail
} // namespace sco

#ifdef SCO_HEADER_ONLY
# include <sco/promise-inl.hpp>
#endif
