#include "ApiServer.h"
#include "routes/EventRoutes.h"
#include "routes/AvailabilityRoutes.h"
#include "routes/StatsRoutes.h"
#include "routes/RecurringRoutes.h"
#include <iostream>

ApiServer::ApiServer(Model &model, int port)
    : model_(model), port_(port) {
    setupRoutes();
}

void ApiServer::start() {
    std::cout << "Starting API server on port " << port_ << std::endl;
    server_.listen("0.0.0.0", port_);
}

void ApiServer::stop() {
    server_.stop();
}

void ApiServer::setupRoutes() {
    server_.Options(R"(.*)", [](const httplib::Request &, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        res.status = 200;
        res.set_content("", "text/plain");
    });

    EventRoutes::registerRoutes(server_, model_);
    AvailabilityRoutes::registerRoutes(server_, model_);
    StatsRoutes::registerRoutes(server_, model_);
    RecurringRoutes::registerRoutes(server_, model_);
}
