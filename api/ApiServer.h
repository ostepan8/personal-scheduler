#pragma once

#include "../model/Model.h"
#include <httplib.h>

// ApiServer exposes scheduler functionality via HTTP endpoints.
// All times in requests/responses are local time strings "YYYY-MM-DD HH:MM".
// JSON responses use the form {"status":"ok","data":...} or {"status":"error","message":...}
class ApiServer {
public:
    ApiServer(Model &model, int port = 8080);
    void start();

private:
    Model &model_;
    int port_;
    httplib::Server server_;

    void setupRoutes();
};
