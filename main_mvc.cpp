#include "controller/Controller.h"
#include "view/TextualView.h"
#include "model/Model.h"
#include "database/SQLiteScheduleDatabase.h"
#include "scheduler/EventLoop.h"
#include <vector>
#include <iostream>

int main() {
    SQLiteScheduleDatabase db("events.db");
    Model model(&db);
    EventLoop loop(model);
    loop.start();

    // schedule tasks for existing events
    auto farFuture = std::chrono::system_clock::now() + std::chrono::hours(24 * 365);
    auto events = model.getEvents(-1, farFuture);
    TextualView view(model);
    Controller controller(model, view, &loop);
    for (const auto &ev : events) {
        controller.scheduleTask(ev);
    }

    // run interactive CLI
    controller.run();
    loop.stop();
    return 0;
}
