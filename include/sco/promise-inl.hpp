#pragma once

#ifndef SCO_HEADER_ONLY
# include <sco/promise.hpp>
#endif

namespace sco::detail {

SCO_INLINE bool promise_shared::release_and_check_await_done() {
    return --await_pending == 0;
}

SCO_INLINE COSTD::coroutine_handle<> promise_shared::handle() {
    return COSTD::coroutine_handle<>::from_address(handle_address);
}

SCO_INLINE promise_shared::promise_shared(int pending, promise_type_base* promise, COSTD::coroutine_handle<> h)
    : await_pending(pending), promise(promise), handle_address(h.address()) {}

SCO_INLINE COSTD::coroutine_handle<> root_result::root_handle() {
    return COSTD::coroutine_handle<>::from_address(root_handle_address);
}

SCO_INLINE sync_object make_sync_object(int pending, promise_type_base& promise, COSTD::coroutine_handle<> h) {
    return std::make_shared<promise_shared>(pending, &promise, h);
}

SCO_INLINE COSTD::coroutine_handle<> promise_type_base::final_awaiter::await_suspend_(COSTD::coroutine_handle<> h, promise_type_base& promise) {
    auto& parent = promise.sync_;
    if (!parent) {
        // If the current coroutine is the root coroutine,
        // save additional results to the thread stack.
        *promise.root_ = root_result{
            promise.exception_,
            h.address(),
        };
        return COSTD::noop_coroutine();
    }

    if (parent->release_and_check_await_done()) {
        // Propagating up.
        if (promise.root_) {
            parent->promise->root_ = promise.root_;
        }
        return parent->handle();
    }
    return COSTD::noop_coroutine();
}

SCO_INLINE void promise_type_base::unhandled_exception() {
    exception_ = std::current_exception();
}

} // namespace sco::detail
