

#include <sco/sco.hpp>

#include <iostream>
#include <future>
#include <vector>

static std::vector<std::future<void>> pending_futures;

void test(int a, int b, const std::function<void(int,int)>& cb) {
    auto h = std::async(std::launch::async, [=]{
        std::this_thread::sleep_for(std::chrono::seconds{5});
        cb(a+1, b+1);
    });
    pending_futures.push_back(std::move(h));
    std::cout << "test return" << std::endl;
}

sco::async<std::pair<int, int>> some2(int a, int b) {
    int c{}, d{};
    co_await sco::call_with_callback(&test, a, b, sco::cb_tie<void(int,int)>(c, d));

    std::cout << "some return" << std::endl;

    co_return std::make_pair(c, d);
}

sco::async<> some(int a, int b) {
    int c{}, d{};
    co_await sco::call_with_callback(&test, a, b, sco::cb_tie<void(int,int)>(c, d));

    std::cout << c << ',' << d << std::endl;

    std::tie(c, d) = co_await some2(c, d);

    std::cout << c << ',' << d << std::endl;
}

int main() {
    // testfunc()();
    some(1, 2).start_root_in_this_thread();
    std::cout << "some end" << std::endl;

    std::this_thread::sleep_for(std::chrono::hours{1});

    return 0;
}
