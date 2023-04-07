

#include <sco/sco.hpp>

#include <iostream>
#include <future>
#include <vector>

using namespace std::chrono_literals;

namespace {
std::vector<std::future<void>> pending_futures;

void plus_async(int a, int b, const std::function<void(int)>& cb) {
    auto h = std::async(std::launch::async, [=]{
        std::this_thread::sleep_for(1s);
        cb(a + b);
    });
    pending_futures.push_back(std::move(h));

    std::cout << "plus_async return" << std::endl;
}

void mul_async(int a, int b, const std::function<void(int)>& cb) {
    auto h = std::async(std::launch::async, [=]{
        std::this_thread::sleep_for(1s);
        cb(a * b);
    });
    pending_futures.push_back(std::move(h));

    std::cout << "mul_async return" << std::endl;
}

// Feels like a sleep function in some asynchronous frameworks.
void delay_async(const std::chrono::milliseconds& ms, const std::function<void()>& cb) {
    auto h = std::async(std::launch::async, [=]{
        std::this_thread::sleep_for(ms);
        cb();
    });
    pending_futures.push_back(std::move(h));

    std::cout << "delay_async return" << std::endl;
}

sco::async<int> plus(int a, int b) {
    int c{};
    co_await sco::call_with_callback(&plus_async, a, b, sco::cb_tie<void(int)>(c));
    co_return c;
}

sco::async<int> mul(int a, int b) {
    int c{};
    co_await sco::call_with_callback(&mul_async, a, b, sco::cb_tie<void(int)>(c));
    co_return c;
}

sco::async<> delay(const std::chrono::milliseconds& ms) {
    co_await sco::call_with_callback(&delay_async, ms, sco::cb_tie<void()>());
    co_return;
}

// Some third-party libraries have implemented the awaiter interface.
struct other_awaiter_return_99 {
    constexpr bool await_ready() const noexcept { return true; }
    constexpr void await_suspend(COSTD::coroutine_handle<> h) const noexcept {}
    constexpr int await_resume() const noexcept { return 99; }
};

// (a * b) * c
sco::async<> test1(int a, int b, int c) {
    auto d = co_await plus(a, b);
    std::cout << "test1 " << a << " + " << b << " = " << d << std::endl;

    // check call sco::all with only one parameter.
    co_await sco::all(delay(1s));

    auto e = co_await mul(c, d);
    std::cout << "test1 " << c << " * " << d << " = " << e << std::endl;

    std::cout << "test1 finish" << std::endl;
    co_return;
}

// a * b, a * b
sco::async<int> test2(int a, int b) {
    // sco::all with multiple futures.
    auto r = co_await sco::all(plus(a, b),
        // call_with_callback can be used in sco::all.
        sco::call_with_callback(&delay_async, 1s, sco::cb_tie<void()>()),
        mul(a, b),
        // three-party awaiter can be used in sco::all.
        other_awaiter_return_99{});
    std::cout << "test2 " << a << " + " << b << " = " << std::get<0>(r) << std::endl;
    // the 2nd `FutureLike` return type is void,
    // so the return value is ignored.
    std::cout << "test2 " << a << " * " << b << " = " << std::get<1>(r) << std::endl;
    std::cout << "test2 other_awaiter " << std::get<2>(r) << std::endl;

    std::cout << "test2 finish" << std::endl;
    co_return 1;
}

// a * b, a * b
sco::async<bool> test3(int a, int b) {
    std::vector<sco::async<int>> asyncs;
    asyncs.push_back(plus(a, b));
    asyncs.push_back(mul(a, b));
    auto r = co_await sco::all(asyncs.begin(), asyncs.end());

    std::cout << "test3 " << a << " + " << b << " = " << r[0] << std::endl;
    std::cout << "test3 " << a << " * " << b << " = " << r[1] << std::endl;

    std::cout << "test3 finish" << std::endl;
    co_return true;
}

sco::async<> root() {
    // any async type can be converted to sco::async<>.
    std::vector<sco::async<void>> asyncs;
    asyncs.push_back(test1(1, 2, 3));
    asyncs.emplace_back(test2(4, 5));
    asyncs.emplace_back(test3(6, 7));
    co_await sco::all(asyncs.begin(), asyncs.end());
}

} // namespace

int main() {
    root().start_root_in_this_thread();
    std::cout << "main thread end" << std::endl;

    std::this_thread::sleep_for(5s);
    return 0;
}
