

#include "sco.hpp"

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

sco::async<> some(int a, int b) {
    int c{}, d{};
    co_await sco::call_with_callback(&test, a, b, sco::cb<void(int,int)>(c, d));

    std::cout << c << ',' << d << std::endl;
}

int main() {
    // testfunc()();
    some(1, 2).start_root_in_this_thread();
    std::cout << "some end" << std::endl;
    return 0;
}
