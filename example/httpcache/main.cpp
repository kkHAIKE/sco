#include <sco/sco.hpp>
#include <hv/HttpServer.h>
#include <hv/http_client.h>
#include <hv/HttpMessage.h>
#include <hv/requests.h>
#include <sw/redis++/async_redis++.h>

#if 0
// redis++ has not support coroutine yet,
// but <coroutine> is not available in some compilers
#include <sw/redis++/co_redis++.h>
#endif

#include <iostream>

using namespace sw;

namespace {

const int Port = 8888;
const char * const RemoteUrl = "https://www.oschina.net";
const char * const LocalRedis = "tcp://127.0.0.1:6379";

// wrap http_client_send_async to sco::async
sco::async<HttpResponsePtr> client_async(const HttpRequestPtr& req) {
    HttpResponsePtr ret;
    co_await sco::call_with_callback(&requests::async, req,
        sco::cb_tie<void(const HttpResponsePtr&)>(ret));
    co_return ret;
}

auto& get_redis() {
    static auto ret = redis::AsyncRedis(LocalRedis);
    return ret;
}

sco::async<redis::OptionalString> redis_get_async(const redis::StringView& key) {
    redis::Future<redis::OptionalString> ret;
    auto cb = sco::cb_tie<void(redis::Future<redis::OptionalString>&&)>(sco::wmove(ret)); // use move assignment
    co_await sco::call_with_callback(&redis::AsyncRedis::get<decltype(cb)>, get_redis(), key, std::move(cb));
    co_return ret.get();
}

sco::async<bool> redis_set_async(const redis::StringView& key, const redis::StringView& value,
    const std::chrono::milliseconds &ttl)
{
    redis::Future<bool> ret;
    auto cb = sco::cb_tie<void(redis::Future<bool>&&)>(sco::wmove(ret)); // use move assignment
    // use lambda to resolve the problem of overload resolution
    co_await sco::call_with_callback([&](decltype(cb)&& cb) {
        get_redis().set(key, value, ttl, std::move(cb));
    }, std::move(cb));
    co_return ret.get();
}

} // namespace

int main() {
    hv::HttpService router;

    router.GET("/", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) {
        // It is better to pass coroutine parameters by value.
        [](HttpRequestPtr req, HttpResponseWriterPtr writer) -> sco::async<> {
            // read from cache first
            auto val = co_await redis_get_async(req->FullPath());
            if (val) {
                // hit
                std::cout << "hit: " << req->FullPath() << std::endl;

                writer->Begin();
                writer->WriteHeader("Content-Type", "text/html");
                writer->WriteBody(*val);
                writer->End();
                co_return;
            }

            // miss, request from remote
            auto req2 = std::make_shared<HttpRequest>();
            req2->scheme = "https";
            req2->url = RemoteUrl + req->FullPath();
            auto resp = co_await client_async(req2);

            // write response content, call it later.
            auto write_resp = [&]() -> sco::async<> {
                writer->Begin();
                if (resp) {
                    resp->headers.erase("Transfer-Encoding");
                    writer->WriteResponse(resp.get());
                } else {
                    writer->WriteStatus(HTTP_STATUS_NOT_FOUND);
                    writer->WriteHeader("Content-Type", "text/html");
                    writer->WriteBody("<center><h1>404 Not Found</h1></center>");
                }
                writer->End();
                // must call co_return explicitly
                co_return;
            }; // Do not call the lambda immediately here and return the coroutine,
            // it should be called at the co_await location instead.

            // write html to cache and write response
            if (resp && resp->status_code == HTTP_STATUS_OK && resp->ContentType() == TEXT_HTML) {
                co_await sco::all(
                    redis_set_async(req->FullPath(), resp->body, std::chrono::seconds(30)),
                    write_resp());
                co_return;
            }

            co_await write_resp();

            // must call co_return explicitly
            co_return;
        // can not use capture list in lambda coroutines within the thread context.
        }(req, writer).start_root_in_this_thread();
    });

    hv::HttpServer server;
    server.registerHttpService(&router);
    server.setPort(Port);
    server.setThreadNum(4);

    std::cout << "start http://127.0.0.1:" << Port << std::endl;
    server.run();

    return 0;
}
