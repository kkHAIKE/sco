#pragma once

#include <sco/async.hpp>

namespace sco {
namespace detail {

class async_handle {
private:
    void* h_address_{};
    COSTD::coroutine_handle<> handle();

    promise_type_base* promise_{};

public:

    // no-copytable
    async_handle(const async_handle&) = delete;
    async_handle& operator=(const async_handle&) = delete;
    async_handle& operator=(async_handle&&) = delete;

    template<typename Ret>
    async_handle(async<Ret>&& other) noexcept {
        h_address_ = other.h_.address();
        promise_ = &other.h_.promise();
        other.h_ = typename async<Ret>::handle_type{};
    }

    async_handle(async_handle&& other) noexcept;
    ~async_handle();

private:
    constexpr int pending_count() const noexcept { return 1; }
    void set_sync_object(const sync_object& sync);
    void resume();
    constexpr void return_value() const noexcept {}
    std::exception_ptr return_exception();

    friend future_caller;
};

} // namespace detail

using async_vector = std::vector<detail::async_handle>;

} // namespace sco

#ifdef SCO_HEADER_ONLY
# include <sco/all-inl.hpp>
#endif
