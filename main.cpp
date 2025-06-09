// main.cpp
#include "model/Model.h"
#include "database/SQLiteScheduleDatabase.h"
#include "api/ApiServer.h"
#include "scheduler/EventLoop.h"
#include "calendar/GoogleCalendarApi.h"
#include <vector>
#include <memory>

int main()
{
    // Construct database and model using dependency injection
    SQLiteScheduleDatabase db("events.db");
    Model model(&db);
    auto gcal = std::make_shared<GoogleCalendarApi>("service_account.json");
    model.addCalendarApi(gcal);

    EventLoop loop(model);
    loop.start();

    // Start the HTTP API server
    ApiServer api(model);
    api.start();

    loop.stop();

    return 0;
}
