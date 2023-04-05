#pragma once

#include <sco/promise.hpp>

#include <functional>

namespace sco {

namespace detail {

void start_root_in_this_thread(promise_type_base* promise, COSTD::coroutine_handle<> h,
    const std::function<void()>& clr);

} // namespace detail

template<typename=void>
class async;

// coroutine type
template<typename Ret>
class async {
public:
    using promise_type = detail::promise_type<async, Ret>;
    using handle_type = typename promise_type::handle_type;

private:
    handle_type h_;

public:

    explicit async(handle_type&& h): h_(std::move(h)) {}

    // no-copytable
    async(const async&) = delete;
    async& operator=(const async&) = delete;
    async& operator=(async&&) = delete;

    async(async&& other) noexcept {
        h_ = std::move(other.h_);
        other.h_ = handle_type{};
    }

    ~async() {
        if (h_) {
            h_.destroy();
        }
    }

    void start_root_in_this_thread() {
        detail::start_root_in_this_thread(&h_.promise(), h_, [this] { h_ = handle_type{}; });
    }

private:
    constexpr int pending_count() const noexcept { return 1; }
    void set_sync_object(const detail::sync_object& sync) {
        h_.promise().set_sync_object_from_future(sync);
    }
    void resume() { h_.resume(); }
    Ret return_value() {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        return std::move(*h_.promise().value_);
    }
    std::exception_ptr return_exception() { return h_.promise().exception_; }

    friend detail::future_caller;
};

template<>
class async<void> {
public:
    using promise_type = detail::promise_type<async, void>;
    using handle_type = typename promise_type::handle_type;

private:
    COSTD::coroutine_handle<> h_;
    detail::promise_type_base* promise_{};

public:
    explicit async(handle_type&& h): h_(h), promise_(&h.promise()) {}

    // no-copytable
    async(const async&) = delete;
    async& operator=(const async&) = delete;
    async& operator=(async&&) = delete;

    template<typename Ret>
    async(async<Ret>&& other) noexcept {
        h_ = other.h_;
        promise_ = &other.h_.promise();
        other.h_ = typename async<Ret>::handle_type{};
    }

    template<>
    async(async<void>&& other) noexcept;
    ~async();

    void start_root_in_this_thread();

private:
    constexpr int pending_count() const noexcept { return 1; }
    void set_sync_object(const detail::sync_object& sync);
    void resume();
    constexpr void return_value() const noexcept {}
    std::exception_ptr return_exception();

    friend detail::future_caller;
};

} // namespace sco

#ifdef SCO_HEADER_ONLY
# include <sco/async-inl.hpp>
#endif
