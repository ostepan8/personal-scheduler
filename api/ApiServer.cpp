#include "ApiServer.h"
#include "routes/EventRoutes.h"
#include "routes/AvailabilityRoutes.h"
#include "routes/StatsRoutes.h"
#include "routes/RecurringRoutes.h"
#include "routes/TaskRoutes.h"
#include "../utils/BuiltinActions.h"
#include "../utils/BuiltinNotifiers.h"
#include "../utils/EnvLoader.h"
#include "../security/Auth.h"
#include "../security/RateLimiter.h"
#include <iostream>

ApiServer::ApiServer(Model &model, int port, const std::string &host, EventLoop *loop)
    : model_(model), port_(port), host_(host), loop_(loop) {
    const char *key = getenv("API_KEY");
    const char *adm = getenv("ADMIN_API_KEY");
    if (key) auth_ = std::make_unique<Auth>(key, adm ? adm : "");
    const char *limit = getenv("RATE_LIMIT");
    size_t maxReq = limit ? std::stoul(limit) : 100;
    const char *window = getenv("RATE_WINDOW");
    int sec = window ? std::stoi(window) : 60;
    limiter_ = std::make_unique<RateLimiter>(maxReq, std::chrono::seconds(sec));
    const char *cors = getenv("CORS_ORIGIN");
    if (cors) corsOrigin_ = cors;

    BuiltinActions::registerAll();
    BuiltinNotifiers::registerAll();

    setupRoutes();
}

void ApiServer::start() {
    std::cout << "Starting API server on " << host_ << ":" << port_ << std::endl;
    server_.listen(host_.c_str(), port_);
}

void ApiServer::stop() {
    server_.stop();
}

void ApiServer::setupRoutes() {
    server_.set_pre_routing_handler([this](const httplib::Request &req, httplib::Response &res) -> httplib::Server::HandlerResponse {
        if (limiter_ && !limiter_->allow(req.remote_addr)) {
            std::cerr << "Rate limit exceeded from " << req.remote_addr << std::endl;
            res.status = 429;
            res.set_content("{\"status\":\"error\",\"message\":\"Too Many Requests\"}", "application/json");
            return httplib::Server::HandlerResponse::Handled;
        }
        if (auth_ && !auth_->authorize(req)) {
            std::cerr << "Unauthorized request from " << req.remote_addr << std::endl;
            res.status = 401;
            res.set_content("{\"status\":\"error\",\"message\":\"Unauthorized\"}", "application/json");
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    server_.set_post_routing_handler([this](const httplib::Request &, httplib::Response &res) {
        res.set_header("X-Content-Type-Options", "nosniff");
        res.set_header("X-Frame-Options", "DENY");
        res.set_header("X-XSS-Protection", "1; mode=block");
        if (!corsOrigin_.empty()) {
            res.set_header("Access-Control-Allow-Origin", corsOrigin_.c_str());
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        }
    });

    server_.Options(R"(.*)", [this](const httplib::Request &, httplib::Response &res) {
        if (!corsOrigin_.empty()) {
            res.set_header("Access-Control-Allow-Origin", corsOrigin_.c_str());
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        }
        res.status = 200;
        res.set_content("", "text/plain");
    });

    EventRoutes::registerRoutes(server_, model_);
    AvailabilityRoutes::registerRoutes(server_, model_);
    StatsRoutes::registerRoutes(server_, model_);
    RecurringRoutes::registerRoutes(server_, model_);
    TaskRoutes::registerRoutes(server_, model_, loop_);
}
