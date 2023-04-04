#pragma once

#include <sco/promise.hpp>
#include <sco/callback.hpp>

namespace sco {

// coroutine type
template<typename Ret=void>
class async { //: public detail::future_trait<async> {
public:
    using promise_type = detail::promise_type<async, Ret>;
    using handle_type = typename promise_type::handle_type;

private:
    handle_type h_;
    bool skip_destroy_{};

public:

    explicit async(handle_type&& h): h_(std::move(h)) {}

    // no-copytable
    async(const async&) = delete;
    async& operator=(const async&) = delete;
    async& operator=(async&&) = delete;

    async(async&& other) noexcept {
        h_ = std::move(other.h_);
        skip_destroy_ = other.skip_destroy_;
        other.skip_destroy_ = true;
    }

    ~async() {
        if (!skip_destroy_) {
            h_.destroy();
        }
    }

    void start_root_in_this_thread() {
        detail::root_result::opt res;
        h_.promise().set_root_result_from_thread(res);
        h_.resume();

        if (!res) {
            skip_destroy_ = true;
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

private:
    constexpr int pending_count() const noexcept { return 1; }
    void set_sync_object(const detail::sync_object& sync) { h_.promise().sync_ = sync; }
    void resume() { h_.resume(); }
    Ret return_value() {
        if constexpr (!std::is_void_v<Ret>) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            return std::move(*h_.promise().value_);
        }
    }
    std::exception_ptr return_exception() { return h_.promise().exception_; }

    friend detail::future_caller;
};

} // namespace sco
