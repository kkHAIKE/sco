#pragma once

#ifndef SCO_HEADER_ONLY
# include <sco/async.hpp>
#endif

namespace sco {
namespace detail {

SCO_INLINE void start_root_in_this_thread(promise_type_base* promise, const COSTD::coroutine_handle<>& h,
    const std::function<void()>& clr) {
    detail::root_result::opt res;
    promise->set_root_result_from_thread(res);
    h.resume();

    if (!res) {
        // reset the root coroutine.
        clr();
    }

    if (res && res->exception) {
        std::rethrow_exception(res->exception);
    }

    // if constexpr (!std::is_void_v<Ret>) {
    //     if (res) {
    //         return std::move(*h_.promise().value_);
    //     } else {
    //         throw std::runtime_error("thread need return but switched");
    //     }
    // }
}

} // namespace detail

SCO_INLINE async<void>& async<void>::operator=(async&& other) noexcept {
    h_ = other.h_;
    promise_ = other.promise_;
    other.h_ = COSTD::coroutine_handle<>{};
    return *this;
}

SCO_INLINE async<void>::~async() {
    if (h_) {
        h_.destroy();
    }
}

SCO_INLINE void async<void>::start_root_in_this_thread() {
    detail::start_root_in_this_thread(promise_, h_, [this] { h_ = COSTD::coroutine_handle<>{}; });
}

SCO_INLINE void async<void>::set_sync_object(const detail::sync_object& sync) {
    promise_->set_sync_object_from_future(sync);
}

SCO_INLINE void async<void>::resume() {
    h_.resume();
}

SCO_INLINE std::exception_ptr async<void>::return_exception() {
    return promise_->exception_;
}

} // namespace sco
