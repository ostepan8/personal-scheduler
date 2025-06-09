#include "controller/Controller.h"
#include "view/TextualView.h"
#include "model/Model.h"
#include "database/SQLiteScheduleDatabase.h"
#include "scheduler/EventLoop.h"
#include "scheduler/ScheduledTask.h"
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
    for (const auto &ev : events) {
        auto task = std::make_shared<ScheduledTask>(
            ev.getId(), ev.getDescription(), ev.getTitle(), ev.getTime(), ev.getDuration(),
            std::chrono::minutes(10),
            [id = ev.getId(), title = ev.getTitle()](){
                std::cout << "[" << id << "] \"" << title << "\" notification\n";
            },
            [id = ev.getId(), title = ev.getTitle()](){
                std::cout << "[" << id << "] \"" << title << "\" executing\n";
            });
        loop.addTask(task);
    }

    TextualView view(model);
    Controller controller(model, view, &loop);
    controller.run();
    loop.stop();
    return 0;
}
