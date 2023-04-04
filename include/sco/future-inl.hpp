#pragma once

#ifndef SCO_HEADER_ONLY
# include <sco/future.hpp>
#endif

namespace sco::detail {

SCO_INLINE std::exception_ptr future_base::return_exception() {
    return exception_;
}

SCO_INLINE void future_with_sync::set_sync_object(const sync_object& sync) {
    sync_ = sync;
}

} // namespace sco::detail
