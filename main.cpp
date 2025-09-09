// main_solid.cpp - SOLID Principles Implementation
#include "model/Model.h"
#include "database/SQLiteScheduleDatabase.h"
#include "api/ApiServer.h"
#include "services/EventService.h"
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
#include "security/Auth.h"
#include "security/RateLimiter.h"
#include "utils/DependencyContainer.h"
#include <vector>
#include <memory>
#include <chrono>

int main()
{
    // Load configuration from .env if present
    EnvLoader::load();
    
    // Setup dependency injection container (Dependency Inversion Principle)
    DependencyContainer container;
    
    // Register core components following DIP
    auto db = std::make_shared<SQLiteScheduleDatabase>("events.db");
    container.registerSingleton<SQLiteScheduleDatabase>(db);
    
    auto model = std::make_shared<Model>(db.get());
    container.registerSingleton<Model>(model);
    
    // Register calendar API
    auto gcal = std::make_shared<GoogleCalendarApi>("calendar_integration/credentials.json");
    model->addCalendarApi(gcal);
    
    // Register event loop
    auto eventLoop = std::make_shared<EventLoop>(*model);
    container.registerSingleton<EventLoop>(eventLoop);
    eventLoop->start();

    // Register settings and wake scheduler
    auto settings = std::make_shared<SettingsStore>("events.db");
    container.registerSingleton<SettingsStore>(settings);
    
    const char *wakeUrl = getenv("WAKE_SERVER_URL");
    if (wakeUrl) settings->setString("wake.server_url", wakeUrl);
    
    auto wake = std::make_shared<WakeScheduler>(*model, *eventLoop, *settings);
    container.registerSingleton<WakeScheduler>(wake);
    wake->scheduleToday();
    wake->scheduleDailyMaintenance();

    // Re-enqueue persisted task events following the same pattern as before
    {
        using namespace std::chrono;
        BuiltinActions::registerAll();
        BuiltinNotifiers::registerAll();
        auto now = system_clock::now();
        auto horizon = now + hours(24 * 365);
        auto events = model->getEvents(1000, horizon);
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
                task->setNotifierName(ev.getNotifierName());
                task->setActionName(ev.getActionName());
                eventLoop->addTask(task);
            }
        }
    }

    // Setup HTTP server following SOLID principles
    const char *portEnv = getenv("PORT");
    int port = portEnv ? std::stoi(portEnv) : 8080;
    const char *hostEnv = getenv("HOST");
    std::string host = hostEnv ? hostEnv : "127.0.0.1";
    
    // Create optional security components
    Auth* authPtr = nullptr;
    RateLimiter* limiterPtr = nullptr;
    
    const char *key = getenv("API_KEY");
    const char *adm = getenv("ADMIN_API_KEY");
    if (key) {
        auto auth = std::make_shared<Auth>(key, adm ? adm : "");
        container.registerSingleton<Auth>(auth);
        authPtr = auth.get();
    }
    
    const char *limit = getenv("RATE_LIMIT");
    size_t maxReq = limit ? std::stoul(limit) : 100;
    const char *window = getenv("RATE_WINDOW");
    int sec = window ? std::stoi(window) : 60;
    auto limiter = std::make_shared<RateLimiter>(maxReq, std::chrono::seconds(sec));
    container.registerSingleton<RateLimiter>(limiter);
    limiterPtr = limiter.get();
    
    // Create service layer (following DIP - depend on abstractions)
    EventService eventService(*model);
    
    // Create SOLID-compliant API server
    ApiServer api(eventService, port, host, authPtr, limiterPtr);
    
    std::cout << "🏗️  SOLID Principles Architecture:\n";
    std::cout << "   ✅ Single Responsibility: Each class has one reason to change\n";
    std::cout << "   ✅ Open/Closed: Open for extension, closed for modification\n";
    std::cout << "   ✅ Liskov Substitution: Derived classes are substitutable\n";
    std::cout << "   ✅ Interface Segregation: Small, focused interfaces\n";
    std::cout << "   ✅ Dependency Inversion: Depend on abstractions, not concretions\n";
    std::cout << "   📊 Routes registered: " << api.getRouteCount() << "\n";
    
    api.start();

    eventLoop->stop();

    return 0;
}