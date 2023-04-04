#pragma once

#ifndef SCO_HEADER_ONLY
# include <sco/all.hpp>
#endif

namespace sco {
namespace detail {

SCO_INLINE COSTD::coroutine_handle<> async_handle::handle() {
    return COSTD::coroutine_handle<>::from_address(h_address_);
}

SCO_INLINE async_handle::async_handle(async_handle&& other) noexcept {
    h_address_ = other.h_address_;
    promise_ = other.promise_;
    other.h_address_ = nullptr;
}

SCO_INLINE async_handle::~async_handle() {
    if (h_address_) {
        handle().destroy();
    }
}

SCO_INLINE void async_handle::set_sync_object(const sync_object& sync) {
    promise_->set_sync_object_from_future(sync);
}

SCO_INLINE void async_handle::resume() {
    handle().resume();
}

SCO_INLINE std::exception_ptr async_handle::return_exception() {
    return promise_->exception_;
}

} // namespace detail
} // namespace sco
