# sco
The simplest C++20 coroutine library, specifically designed to solve the problem of callback-hell, supports integration with any asynchronous I/O framework

## Install
### Header only version
Copy the include folder to your build tree and use a C++20 compiler.

### Compiled version
**require** cmake version >= 3.12

```cmake
FetchContent_Declare(
    sco
    GIT_REPOSITORY https://github.com/kkHAIKE/sco.git
    GIT_TAG        main
)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.14)
    FetchContent_MakeAvailable(sco)
else()
    FetchContent_GetProperties(sco)
    if (NOT sco_POPULATED)
        FetchContent_Populate(sco)
        add_subdirectory(${sco_SOURCE_DIR} ${sco_BINARY_DIR})
    endif()
endif()

# link with your target
target_link_libraries(your_target sco::sco)
```

## Platforms
compiler|version
-|-
GCC|10
Clang|8
AppleClang|10.0.1
MSVC|2019 (16.8)

## Features
* very **tiny/simple** and no dependencies.
* support **all** async frameworks.
* support 3rd-party libraries have implemented the awaiter interface.
* `sco::call_with_callback` wraps any [async function](#async-function) to make it available for use within a coroutine.
* `sco::all` will wait for all coroutines to complete.

## Usage samples
```c++
#include <sco/sco.hpp>

// a plus async function return value after 1 second
void plus_async(int a, int b, const std::function<void(int)>& cb);

// wWrap the async function into a coroutine using sco:async.
sco::async<int> plus(int a, int b) {
    int c{};
    co_await sco::call_with_callback(&plus_async, a, b, sco::cb_tie<void(int)>(c));
    co_return c;
}

// This is the root coroutine called from a thread.
sc::async<void> root_co(int a, int b) {
    int c = co_await plus(a, b);
    std::cout << a << " + " << b << " = " << c << std::endl;
    // void coroutine must call co_return explicitly.
    co_return;
}

// Some async frameworks will obtain a thread from a thread pool and call this function.
void thread_func_like_handler() {
    root_co(1, 2).start_root_in_this_thread();
    // This thread may quickly return when the first `co_await` is being called.
}
```

Is equivalent to:
```c++
void thread_func_like_handler() {
    plus_async(1, 2, [](int c) {
        std::cout << "1 + 2 = " << c << std::endl;
    });
}
```

more samples in [example.cpp](https://github.com/kkHAIKE/sco/blob/main/example/example.cpp)

## reference
## sco::async
* `sco::async<T>` is a coroutine that returns a value of type `T`.
* `sco::async<>` (aka `sco::async<void>`) can be obtained by converting from any `sco::async<T>`.
* `start_root_in_this_thread` will start the coroutine in the current thread.
* is a `FutureLike` type.

## sco::call_with_callback
* `sco::call_with_callback` wraps any [async function](#async-function) to make it available for use within a coroutine.
* **require** `std::co_tie` to tie the callback parameters to the coroutine variables.
* `sco::wmove` use move assignment instead of regular assignment.
    ```c++
    std::co_tie<void(NoCopy)> cb{sco::wmove(x)};
    ```
* is return a `FutureLike` type.

## sco::all
* `sco::all` will wait for all coroutines to complete.
* use with `sco::async` container:
    ```c++
    std::vector<sco::async<int>> coroutines;
    coroutines.emplace_back(plus(1, 2));
    coroutines.emplace_back(plus(3, 4));

    auto ret = co_await sco::all(coroutines.begin(), coroutines.end());

    std::cout << ret[0] << std::endl; // 3
    std::cout << ret[1] << std::endl; // 7
    ````
* use with any `FutureLike`:
    ```c++
    int r{};
    auto ret = co_await sco::all(
        plus(1, 2),
        call_with_callback(&plus_async, 3, 4, sco::cb_tie<void(int)>(r)),
        plus(5, 6),
    );

    std::cout << ret[0] << std::endl; // 3
    std::cout << r << std::endl; // 7
    // the 2nd `FutureLike` return type is void,
    // so the return value is ignored.
    std::cout << ret[1] << std::endl; // 11
    ```
* can use with 3rd-party libraries have implemented the awaiter interface.

## limitations
### async function
* function signature must be like `void (*)(Args..., const std::function<void(Ret...)>&, Args...)`.
* The fact that this callback function is called means that the asynchronous function has completed, thereby transferring control flow.
* the callback function must be called exactly **once** if no exception is thrown.

### root coroutine
* if use reference type as parameter, the reference may be invalid after the frist `co_await`.
    ```c++
    sc::async<void> root_co(Type& x) {
        co_await other_async(x);
        // `x` may be invalid.
        co_return;
    }

    void thread_func_like_handler() {
        Type x;
        root_co(x).start_root_in_this_thread();
    }
    ```

## License
MIT
