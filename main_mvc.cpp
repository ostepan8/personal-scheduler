#include "controller/Controller.h"
#include "view/TextualView.h"
#include "model/Model.h"
#include "database/SQLiteScheduleDatabase.h"
#include "scheduler/EventLoop.h"
#include "calendar/GoogleCalendarApi.h"
#include <vector>
#include <memory>
#include <iostream>
#include <cstdlib>
#include "utils/NotificationRegistry.h"
#include "utils/ActionRegistry.h"
#include "utils/BuiltinNotifiers.h"
#include "utils/BuiltinActions.h"
#include "scheduler/ScheduledTask.h"
#include "database/SettingsStore.h"
#include "processing/WakeScheduler.h"

int main()
{
    std::cout << "[mvc] opening DB...\n";
    SQLiteScheduleDatabase db("events.db");
    std::cout << "[mvc] creating model...\n";
    Model model(&db);
    // MVC: Google Calendar integration is disabled by default to avoid interactive OAuth in CLI.
    // Set ENABLE_GCAL_MVC=1 explicitly to enable.
    const char *enableGcalMvc = getenv("ENABLE_GCAL_MVC");
    if (enableGcalMvc && std::string(enableGcalMvc) == "1") {
        auto gcal = std::make_shared<GoogleCalendarApi>("calendar_integration/credentials.json");
        if (gcal->testConnection()) {
            model.addCalendarApi(gcal);
        } else {
            std::cout << "Google Calendar integration disabled (connection test failed)\n";
        }
    } else {
        std::cout << "Google Calendar integration disabled (set ENABLE_GCAL_MVC=1 to enable)\n";
    }
    std::cout << "[mvc] constructing view/controller...\n";
    std::cout << "[mvc] creating view...\n";
    TextualView view(model);
    std::cout << "[mvc] creating controller...\n";
    EventLoop loop(model);
    Controller controller(model, view, &loop);
    std::cout << "[mvc] controller ready.\n";
    // fetch events before starting loop
    std::cout << "[mvc] fetching events...\n";
    auto nowTs = std::chrono::system_clock::now();
    auto farFuture = nowTs + std::chrono::hours(24 * 365);
    setenv("DEBUG_MVC", "1", 0);
    auto events = model.getEvents(-1, farFuture);
    std::cout << "[mvc] fetched events count=" << events.size() << "\n";

    // Register built-ins (for CLI addtask and any re-enqueue below)
    BuiltinActions::registerAll();
    BuiltinNotifiers::registerAll();

    // Wake scheduling (same as server)
    SettingsStore settings("events.db");
    const char *wakeUrl = getenv("WAKE_SERVER_URL");
    if (wakeUrl && !settings.getString("wake.server_url")) settings.setString("wake.server_url", wakeUrl);
    WakeScheduler wake(model, loop, settings);
    // Start event loop now
    std::cout << "[mvc] starting event loop...\n";
    loop.start();
    wake.scheduleToday();
    wake.scheduleDailyMaintenance();
    std::cout << "[mvc] scheduling persisted tasks...\n";
    for (const auto &ev : events)
    {
        if (ev.getCategory() == "task") {
            controller.scheduleTask(ev);
        }
    }

    // run interactive CLI
    std::cout << "[mvc] entering run loop...\n";
    controller.run();
    std::cout << "[mvc] stopping loop...\n";
    loop.stop();
    std::cout << "[mvc] exiting...\n";
    return 0;
}
