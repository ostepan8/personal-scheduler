// main.cpp
#include "model/Model.h"
#include "database/SQLiteScheduleDatabase.h"
#include "api/ApiServer.h"
#include "scheduler/EventLoop.h"
#include "scheduler/ScheduledTask.h"
#include "calendar/GoogleCalendarApi.h"
#include "utils/EnvLoader.h"
#include "database/SettingsStore.h"
#include "processing/WakeScheduler.h"
#include "utils/NotificationRegistry.h"
#include "utils/ActionRegistry.h"
#include "utils/BuiltinNotifiers.h"
#include "utils/BuiltinActions.h"
#include <vector>
#include <memory>
#include <chrono>

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

    // Settings + wake-up scheduling
    SettingsStore settings("events.db");
    // Default wake URL from env (overwrite to ensure it's current)
    const char *wakeUrl = getenv("WAKE_SERVER_URL");
    if (wakeUrl) settings.setString("wake.server_url", wakeUrl);
    WakeScheduler wake(model, loop, settings);
    wake.scheduleToday();
    wake.scheduleDailyMaintenance();

    // Re-enqueue persisted task events (category == "task") so their
    // notifications/actions trigger after a restart. Uses default console
    // notify and execute callbacks if none were persisted.
    {
        using namespace std::chrono;
        // Ensure builtins are registered before looking up names
        BuiltinActions::registerAll();
        BuiltinNotifiers::registerAll();
        auto now = system_clock::now();
        auto horizon = now + hours(24 * 365);
        auto events = model.getEvents(-1, horizon);
        for (const auto &ev : events)
        {
            if (ev.getCategory() == "task" && ev.getTime() > now)
            {
                std::vector<system_clock::time_point> notifyTimes;
                if (ev.getTime() - now >= minutes(10))
                {
                    auto tp = ev.getTime() - minutes(10);
                    if (tp > now)
                        notifyTimes.push_back(tp);
                }

                // Look up persisted notifier/action names; fallback to console/defaults
                std::function<void(const std::string&, const std::string&)> notifierFn;
                if (!ev.getNotifierName().empty()) {
                    notifierFn = NotificationRegistry::getNotifier(ev.getNotifierName());
                }
                auto notifyCb = [id = ev.getId(), title = ev.getTitle(), notifierFn]() {
                    if (notifierFn) notifierFn(id, title);
                    else std::cout << "[" << id << "] \"" << title << "\" notification" << std::endl;
                };

                std::function<void()> actionFn;
                if (!ev.getActionName().empty()) {
                    actionFn = ActionRegistry::getAction(ev.getActionName());
                }
                auto actionCb = [id = ev.getId(), title = ev.getTitle(), actionFn]() {
                    if (actionFn) actionFn();
                    else std::cout << "[" << id << "] \"" << title << "\" executing" << std::endl;
                };

                auto task = std::make_shared<ScheduledTask>(
                    ev.getId(), ev.getDescription(), ev.getTitle(), ev.getTime(), ev.getDuration(),
                    notifyTimes, std::move(notifyCb), std::move(actionCb));
                task->setCategory("task");
                // Preserve names for persistence (clone copies base fields)
                task->setNotifierName(ev.getNotifierName());
                task->setActionName(ev.getActionName());
                loop.addTask(task);
            }
        }
    }

    // Start the HTTP API server
    const char *portEnv = getenv("PORT");
    int port = portEnv ? std::stoi(portEnv) : 8080;
    const char *hostEnv = getenv("HOST");
    std::string host = hostEnv ? hostEnv : "127.0.0.1";
    ApiServer api(model, port, host, &loop, &wake, &settings);
    api.start();

    loop.stop();

    return 0;
}
