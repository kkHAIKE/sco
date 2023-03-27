#pragma once

#include "assign.hpp"

#if __has_include(<coroutine>)
#include <coroutine>
#define _STD std
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#define _STD std::experimental
#else
warning <coroutine> not found
#endif

#include <optional>

namespace sco {
namespace detail {

struct promise_type_base;

// 在 co_await 时创建用来传递父协程信息，类似于智能脂针
struct promise_shared {
    using ptr = std::shared_ptr<promise_shared>;

    // 该协程的 done，用来控制以下脂针访问，在 co_await 时创建
    // 由 transform 填充
    std::atomic_int await_done;

    promise_type_base* promise{};
    void *handler{};
};

// 在线程代码内判断是否在当前线程结束
struct root_result {
    using opt = std::optional<root_result>;

    std::exception_ptr exception;
};

// 改动了跟多版本，暂时用这个名字
using sync_object = promise_shared::ptr;

// callback 必须要知道 该协程 的 done -> promise

// 对接 co_await 的基本式，可以继承，也可以是 like 形式
// int done_count() 需要向父要的 done 数目
// void set_sync_object(const sync_object& sync) // future 会保存然后自动填充 done 的数目
// void resume()
// Ret return_value()
// std::exception_ptr return_exception()

struct future_base {
    sync_object sync_;
    std::exception_ptr exception_;

    constexpr int done_count() const noexcept { return 1; }
    void set_sync_object(const sync_object& sync) { sync_ = sync; }
    std::exception_ptr return_exception() { return exception_; }
};

template<typename Ret>
struct future_value: public future_base {
    std::optional<Ret> value_;

    Ret return_value() { return std::move(*value_); }
};

template<>
struct future_value<void>: public future_base {
    constexpr void return_value() const noexcept {}
};

struct future_caller {
    template<typename T>
    inline static int done_count(T& x) {
        return x.done_count();
    }
    template<typename T>
    inline static void set_sync_object(T& x, const sync_object& sync) {
        return x.set_sync_object(sync);
    }
    template<typename T>
    inline static void resume(T& x) {
        x.resume();
    }
    template<typename T>
    inline static auto return_value(T& x) {
        return x.return_value();
    }
    template<typename T>
    inline static std::exception_ptr return_exception(T& x) {
        return x.return_exception();
    }
};

} // detail

namespace detail {

struct promise_type_base {
    constexpr _STD::suspend_always initial_suspend() const noexcept { return {}; }

    root_result::opt* root_{};

    // 子携程自动继续父协程
    struct final_awaiter {
        constexpr bool await_ready() const noexcept { return false; }

        template<typename Child>
        _STD::coroutine_handle<> await_suspend(_STD::coroutine_handle<Child> h) noexcept {
            auto& promise = h.promise();
            auto& parent = promise.sync_;
            if (!parent) {
                // 自身就是 root，当前已经是最后阶段
                *promise.root_ = root_result{promise.exception_};
                return _STD::noop_coroutine();
            }

            if (--parent->await_done == 0) {
                // 向上传递
                if (promise.root_) {
                    parent->promise->root_ = promise.root_;
                }
                return _STD::coroutine_handle<>::from_address(parent->handler);
            }
            return _STD::noop_coroutine();
        }

        constexpr void await_resume() const noexcept {}
    };
    constexpr final_awaiter final_suspend() const noexcept { return {}; }

    std::exception_ptr exception_;
    void unhandled_exception() {
        exception_ = std::current_exception();
    }

    sync_object sync_;

    template<typename Child>
    sync_object make_sync_object(int done, _STD::coroutine_handle<Child>& h) {
        auto ret = std::make_shared<promise_shared>();
        ret->await_done = done;
        ret->promise = this;
        ret->handler = h.address();
        return ret;
    }

    template<typename Future>
    struct future_awaiter {
        using Ret = typename Future::return_type;
        Future fut;

        constexpr bool await_ready() const noexcept { return false; }

        template<typename Child>
        bool await_suspend(_STD::coroutine_handle<Child> h) {
            auto sync = h.promise().make_sync_object(future_caller::done_count(fut) + 1, h);
            future_caller::set_sync_object(fut, sync);
            future_caller::resume(fut);

            // false 恢复协程
            return !(--sync->await_done == 0);
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
        return future_awaiter<Future>{std::move(fut)};
    }
};

struct callback_base {
    // 不好解决复制中注入的问题，先用这个
    std::shared_ptr<promise_shared::ptr> promise_ptr_ = std::make_shared<promise_shared::ptr>();

    void resume() {
        // 继续运行协程
        auto& promise = *promise_ptr_;
        if (--promise->await_done == 0) {
            root_result::opt res;
            promise->promise->root_ = &res;

            _STD::coroutine_handle<>::from_address(promise->handler).resume();

            if (res && res->exception) {
                std::rethrow_exception(res->exception);
            }
        }
    }
};

template<typename, typename, typename=void>
struct callback_obj;

// 给用户使用封装回调参数
// 尝试使用 std::ignore
// callback 只能是右值传入（为了避免重复使用）
template<typename... Args, typename Refs>
struct callback_obj<void(Args...), Refs,
    std::enable_if_t<std::tuple_size_v<Refs> == sizeof...(Args)>>: public callback_base {

    using sign = void(Args...);

    Refs refs_; // 内部为 T& 或 wrapper<T> 或 wrapper<T>& ??

    // 引用回调参数获取
    constexpr explicit callback_obj(Refs&& refs): refs_(std::move(refs)) {}

    // 原函数签名
    void operator()(Args... args) {
        std::tuple<std::add_lvalue_reference_t<Args>...> argsTuple{args...};

        assign_mutl(refs_, argsTuple, std::make_index_sequence<sizeof...(args)>());

        resume();
    }
};

// 空数组特化
template<>
struct callback_obj<void(), std::tuple<>>: public callback_base {
    using sign = void();

    // 引用回调参数获取
    constexpr explicit callback_obj(std::tuple<>&&) {}

    // 原函数签名
    void operator()() {
        resume();
    }
};

}

template<typename Sign, typename... Refs>
constexpr auto cb(Refs&&... refs) {
    return detail::callback_obj<Sign, std::tuple<Refs...>>({std::forward<Refs>(refs)...});
}

////////////

namespace detail {

template<typename Obj, typename Ret>
struct promise_type: public promise_type_base {
    using handle_type = _STD::coroutine_handle<promise_type>;

    auto get_return_object() {
        return Obj(handle_type::from_promise(*this));
    }

    std::optional<Ret> value_;

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<Ret, T>>>
    void return_value(T&& v) noexcept {
        value_ = std::forward<T>(v);
    }
};

// void 特化
template<typename Obj>
struct promise_type<Obj, void>: public promise_type_base {
    using handle_type = _STD::coroutine_handle<promise_type>;

    auto get_return_object() {
        return Obj(handle_type::from_promise(*this));
    }

    constexpr void return_void() const noexcept {}
};

} // detail

// 协程的参数如果是引用的话，尽量在下个 co_await 之前保存
// 可能会在线程切换后变成 悬垂引用

// 记住协程刚创建好是暂停状态的
// 需要通过 start 启动并获得返回值
// 在协程中使用可以不用 start，让 co_await 启动

template<typename Ret=void>
class async { //: public detail::future_trait<async> {
public:
    using promise_type = detail::promise_type<async, Ret>;
    using handle_type = typename promise_type::handle_type;
    using return_type = Ret;

private:
    handle_type h_;
    bool skip_destroy_{};

public:

    explicit async(handle_type&& h): h_(std::move(h)) {}

    // 不能复制
    async(const async&) = delete;
    async& operator=(const async&) = delete;

    async(async&& other) {
        h_ = std::move(other.h_);
        skip_destroy_ = other.skip_destroy_;
        other.skip_destroy_ = true;
    }

    ~async() {
        if (!skip_destroy_) {
            h_.destroy();
        }
    }

    Ret start_root_in_this_thread() {
        detail::root_result::opt res;
        h_.promise().root_ = &res;
        h_.resume();

        if (!res) {
            skip_destroy_ = true;
        }

        if (res && res->exception) {
            std::rethrow_exception(res->exception);
        }

        if constexpr (!std::is_void_v<Ret>) {
            if (res) {
                return std::move(*h_.promise().value_);
            } else {
                throw std::runtime_error("thread need return but switched");
            }
        }
    }

private:
    constexpr int done_count() const noexcept { return 1; }
    void set_sync_object(const detail::sync_object& sync) { h_.promise().sync_ = sync; }
    void resume() { h_.resume(); }
    Ret return_value() {
        if constexpr (!std::is_void_v<Ret>) {
            return std::move(*h_.promise().value_);
        }
    }
    std::exception_ptr return_exception() { return h_.promise().exception_; }

    friend detail::future_caller;
};

namespace detail {

template<typename T>
constexpr auto get_callback_base(T&& v) {
    if constexpr (std::is_base_of_v<callback_base, std::remove_cvref_t<T>>) {
        // 必须是右值引用
        static_assert(!std::is_reference_v<T>, "callback parameter must be right ref");
        return std::tuple<callback_base&>{v};
    } else {
        return std::tuple<>{};
    }
}

}

// 使用 最晚执行 占有规则
// 如果有多个回调，一定只有一个会执行到（分支选择回调）
template<typename F, typename... Args>
auto call_with_callback(F&& f, Args&&... args) {
    // callback 应该会在协程序栈中，不恢复的话不会释放
    auto cbs = std::tuple_cat(detail::get_callback_base(std::forward<Args>(args))...);
    // 参数必须要有 callback_base 类型
    static_assert(std::tuple_size_v<decltype(cbs)> > 0, "call_with_callback must be call with a callback");

    auto cb = std::get<0>(cbs).promise_ptr_;
    auto bf = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

    using CB = decltype(cb);
    using BF = decltype(bf);

    class future_t: protected detail::future_value<void> {
    public:
        using return_type = void;

    private:
        CB cb_;
        BF bf_;

    private:
        void resume() {
            *cb_ = sync_;
            try {
                bf_();
            } catch (...) {
                exception_ = std::current_exception();
            }
        }

        friend detail::future_caller;

    public:
        future_t(CB&& cb, BF&& bf)
            : cb_(std::move(cb)), bf_(std::move(bf)) {}
    };

    return future_t(std::move(cb), std::move(bf));
}

}
