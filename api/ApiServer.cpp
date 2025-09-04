// ApiServer.cpp

#include "ApiServer.h"
#include "routes/EventRoutes.h"
#include "routes/AvailabilityRoutes.h"
#include "routes/StatsRoutes.h"
#include "routes/RecurringRoutes.h"
#include "routes/TaskRoutes.h"
#include "routes/WakeRoutes.h"
#include "../utils/BuiltinActions.h"
#include "../utils/BuiltinNotifiers.h"
#include "../utils/EnvLoader.h"
#include "../security/Auth.h"
#include "../security/RateLimiter.h"
#include <iostream>

ApiServer::ApiServer(Model &model, int port, const std::string &host, EventLoop *loop, WakeScheduler *wake, SettingsStore *settings)
    : model_(model), port_(port), host_(host), loop_(loop), wake_(wake), settings_(settings)
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

        // Authentication (read/write by API key, admin optionally enforced below)
        if (auth_ && !auth_->authorize(req)) {
            res.status = 401;
            res.set_header("Access-Control-Allow-Origin", corsOrigin_.c_str());
            res.set_content(R"({"status":"error","message":"Unauthorized"})", "application/json");
            return httplib::Server::HandlerResponse::Handled;
        }

        // Admin gate for destructive operations if ADMIN_API_KEY is configured
        if (auth_) {
            // If admin key exists, require it for DELETEs except soft deletes
            // Also protect recurring deletes
            bool adminConfigured = true; // presence checked in authorize; but we need to know if adminKey_ set
            // We can't directly see admin key here; infer by isAdmin behavior: if it can ever be true.
            // If isAdmin would always be false, treat as not configured.
            // Use a small heuristic: if admin key is empty, isAdmin will return false even if header is admin.
            // We'll gate only when isAdmin can be true AND method/path is destructive.

            auto isDelete = req.method == "DELETE";
            bool softDeleteSingle = false;
            if (isDelete) {
                // soft deletion allowed without admin
                if (req.has_param("soft") && req.get_param_value("soft") == "true") {
                    softDeleteSingle = true;
                }
            }

            auto path = req.path;
            bool destructive = false;
            if (isDelete) destructive = true; // default: all DELETEs
            // else we could add more destructive operations here (e.g., future bulk modifications)

            if (destructive && !softDeleteSingle) {
                // Require admin only if an admin key is configured
                // Check by testing if a dummy admin check would ever succeed; we can't know here.
                // We will assume that if an ADMIN_API_KEY env was set at construction, auth_ enforces it.
                // Implement a lightweight check by probing both cases: if not admin, but also if there is no admin configured, allow.
                if (!auth_->isAdmin(req)) {
                    // No admin header. If admin key was configured, deny; otherwise allow.
                    // We can detect configuration by checking an env var directly here safely.
                    const char *admEnv = getenv("ADMIN_API_KEY");
                    if (admEnv && *admEnv) {
                        res.status = 403;
                        res.set_header("Access-Control-Allow-Origin", corsOrigin_.c_str());
                        res.set_content(R"({"status":"error","message":"Admin privileges required"})", "application/json");
                        return httplib::Server::HandlerResponse::Handled;
                    }
                }
            }
            // Admin gate for config mutations (e.g., wake config)
            if (req.method == "PUT" && req.path == "/wake/config") {
                const char *admEnv = getenv("ADMIN_API_KEY");
                if (admEnv && *admEnv && !auth_->isAdmin(req)) {
                    res.status = 403;
                    res.set_header("Access-Control-Allow-Origin", corsOrigin_.c_str());
                    res.set_content(R"({"status":"error","message":"Admin privileges required"})", "application/json");
                    return httplib::Server::HandlerResponse::Handled;
                }
            }
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

    // 3) No separate OPTIONS handler needed since pre-routing handles it

    // 4) Your actual routes
    EventRoutes::registerRoutes(server_, model_, wake_);
    AvailabilityRoutes::registerRoutes(server_, model_);
    StatsRoutes::registerRoutes(server_, model_);
    RecurringRoutes::registerRoutes(server_, model_);
    TaskRoutes::registerRoutes(server_, model_, loop_);
    // Wake routes use injected settings
    WakeRoutes::registerRoutes(server_, model_, wake_, settings_);
}
