#pragma once

#include "../model/Model.h"
#include "httplib.h"
#include "../security/Auth.h"
#include "../security/RateLimiter.h"
#include "../scheduler/EventLoop.h"

// ApiServer exposes scheduler functionality via HTTP endpoints.
// All times in requests/responses are local time strings "YYYY-MM-DD HH:MM".
// JSON responses use the form {"status":"ok","data":...} or {"status":"error","message":...}
class ApiServer
{
public:
    // The server binds to `host` and `port`. Security features like
    // authentication and rate limiting are configured via environment
    // variables loaded by `EnvLoader`.
    ApiServer(Model &model,
              int port = 8080,
              const std::string &host = "127.0.0.1",
              EventLoop *loop = nullptr);
    void start();
    void stop();

private:
    Model &model_;
    int port_;
    std::string host_;
    std::string corsOrigin_;
    httplib::Server server_;
    std::unique_ptr<Auth> auth_;
    std::unique_ptr<RateLimiter> limiter_;
    EventLoop *loop_;

    void setupRoutes();
};
