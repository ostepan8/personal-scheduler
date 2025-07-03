// ApiServer.cpp

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
    : model_(model), port_(port), host_(host), loop_(loop)
{
    // Load keys
    const char *key = getenv("API_KEY");
    const char *adm = getenv("ADMIN_API_KEY");
    if (key)
    {
        auth_ = std::make_unique<Auth>(key, adm ? adm : "");
    }

    // Rate limiter
    const char *limit = getenv("RATE_LIMIT");
    size_t maxReq = limit ? std::stoul(limit) : 100;
    const char *window = getenv("RATE_WINDOW");
    int sec = window ? std::stoi(window) : 60;
    limiter_ = std::make_unique<RateLimiter>(maxReq, std::chrono::seconds(sec));

    // CORS origin (fallback to localhost:3000)
    const char *cors = getenv("CORS_ORIGIN");
    corsOrigin_ = cors ? cors : "http://localhost:3000";
    corsOrigin_ = "*";
    BuiltinActions::registerAll();
    BuiltinNotifiers::registerAll();

    setupRoutes();
}

void ApiServer::start()
{
    std::cout << "Starting API server on " << host_ << ":" << port_ << std::endl;
    server_.listen(host_.c_str(), port_);
}

void ApiServer::stop()
{
    server_.stop();
}

void ApiServer::setupRoutes()
{
    // 1) Pre-routing: Handle CORS preflight and authentication
    server_.set_pre_routing_handler([this](const httplib::Request &req, httplib::Response &res) -> httplib::Server::HandlerResponse
                                    {
        // Handle preflight OPTIONS requests immediately
        if (req.method == "OPTIONS") {
            res.status = 200;
            res.set_header("Access-Control-Allow-Origin", corsOrigin_.c_str());
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
            res.set_header("Access-Control-Max-Age", "86400");
            res.set_content("", "text/plain");
            return httplib::Server::HandlerResponse::Handled;
        }

        // Rate limiting
        if (limiter_ && !limiter_->allow(req.remote_addr)) {
            res.status = 429;
            res.set_header("Access-Control-Allow-Origin", corsOrigin_.c_str());
            res.set_content(R"({"status":"error","message":"Too Many Requests"})", "application/json");
            return httplib::Server::HandlerResponse::Handled;
        }

        // Authentication
        if (auth_ && !auth_->authorize(req)) {
            res.status = 401;
            res.set_header("Access-Control-Allow-Origin", corsOrigin_.c_str());
            res.set_content(R"({"status":"error","message":"Unauthorized"})", "application/json");
            return httplib::Server::HandlerResponse::Handled;
        }

        return httplib::Server::HandlerResponse::Unhandled; });

    // 2) Post-routing: Add CORS and security headers to all responses
    server_.set_post_routing_handler([this](const httplib::Request &req, httplib::Response &res)
                                     {
        // Only add CORS headers if they haven't been set already
        if (res.get_header_value("Access-Control-Allow-Origin").empty()) {
            res.set_header("Access-Control-Allow-Origin", corsOrigin_.c_str());
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        }
        
        // Add security headers
        res.set_header("X-Content-Type-Options", "nosniff");
        res.set_header("X-Frame-Options", "DENY");
        res.set_header("X-XSS-Protection", "1; mode=block"); });

    // 3) Remove the separate OPTIONS handler - it's now handled in pre-routing
    // No separate OPTIONS handler needed since pre-routing handles it

    // 4) Your actual routes
    EventRoutes::registerRoutes(server_, model_);
    AvailabilityRoutes::registerRoutes(server_, model_);
    StatsRoutes::registerRoutes(server_, model_);
    RecurringRoutes::registerRoutes(server_, model_);
    TaskRoutes::registerRoutes(server_, model_, loop_);
}