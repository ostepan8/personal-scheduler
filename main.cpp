// main.cpp
#include "model/Model.h"
#include "database/SQLiteScheduleDatabase.h"
#include "api/ApiServer.h"
#include <vector>

int main()
{
    // Construct database and model using dependency injection
    SQLiteScheduleDatabase db("events.db");
    Model model(&db);

    // Start the HTTP API server
    ApiServer api(model);
    api.start();

    return 0;
}
