// main.cpp
#include "model/Model.h"
#include "database/SQLiteScheduleDatabase.h"
#include "api/ApiServer.h"
#include "scheduler/EventLoop.h"
#include "calendar/GoogleCalendarApi.h"
#include "utils/EnvLoader.h"
#include <vector>
#include <memory>

int main()
{
    // Load configuration from .env if present
    EnvLoader::load();
    // Construct database and model using dependency injection
    SQLiteScheduleDatabase db("events.db");
    Model model(&db);
    auto gcal = std::make_shared<GoogleCalendarApi>("calendar_integration/credentials.json");
    model.addCalendarApi(gcal);

    EventLoop loop(model);
    loop.start();

    // Start the HTTP API server
    const char *portEnv = getenv("PORT");
    int port = portEnv ? std::stoi(portEnv) : 8080;
    const char *hostEnv = getenv("HOST");
    std::string host = hostEnv ? hostEnv : "127.0.0.1";
    ApiServer api(model, port, host);
    api.start();

    loop.stop();

    return 0;
}
