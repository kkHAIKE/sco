#pragma once

#ifndef SCO_HEADER_ONLY
# include <sco/callback.hpp>
#endif

namespace sco::detail {

SCO_INLINE void callback_base::resume() {
    if (!promise->release_and_check_await_done()) {
        return;
    }

    root_result::opt res;
    promise->promise->set_root_result_from_thread(res);

    promise->handle().resume();

    if (res) {
        // destroy the root coroutine.
        res->root_handle().destroy();
    }

    if (res && res->exception) {
        std::rethrow_exception(res->exception);
    }
};

} // namespace sco::detail
