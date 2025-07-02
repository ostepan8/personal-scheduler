# Personal Scheduler

All event timestamps are stored internally in UTC to ensure consistent
behavior across time zones. When interacting with the CLI you provide and
view times in your **local time zone**. The scheduler converts local input
times to UTC for storage and converts them back to local time when events
are displayed.

Summary:
- **Input and output:** local time
- **Internal storage:** UTC

## Running Tests

Use `make test` or run the `./run_all_tests.sh` script to build and execute all unit tests.

## Database Persistence

Events are stored in an SQLite database (`events.db`). On startup the model
loads all events from this database, reconstructing recurring patterns so
commands like `list` and `nextn` work across restarts. The database tests in
`tests/database` verify this behavior.

### Preload Horizon

`Model` can optionally limit how far ahead to preload events from the database.
Pass a number of days as the second constructor argument. A negative value (the
default) loads every stored event:

```cpp
// Only load events occurring in the next 30 days
SQLiteScheduleDatabase db("events.db");
Model m(&db, 30);
```

## Event Loop

The scheduler now includes an active `EventLoop` component. Tasks derived from
`Event` (for example `ScheduledTask`) can register notification and execution
callbacks. The loop runs in a background thread, dispatching notifications a
few minutes before each task executes and invoking the task's action at the
scheduled time.

## Customising Notifications and Actions

`ScheduledTask` allows full control over when notifications occur and what
happens when a task runs. You can supply your own callback functions – perfect
for triggering web requests, talking to a Raspberry Pi or scraping a website.

```cpp
// Notify 5 minutes before and call a custom action
ScheduledTask task(
    "abc", "demo", "Demo Task", when,
    std::chrono::hours(1), std::chrono::minutes(5),
    []{ std::cout << "ring ring"; },
    []{ runMyScript(); });
```

Alternatively an absolute notification time can be provided. `Controller`
exposes `scheduleTask` so CLI or agent code can enqueue tasks with different
callbacks or lead times.

## Action Registry

The CLI includes a simple factory for binding named actions to scheduled
events. Actions are registered via `ActionRegistry::registerAction` and can be
referenced when creating tasks with the `addtask` command. `BuiltinActions`
provides a collection of static helpers and a `registerAll()` method that
registers default actions like `hello` and `fetch_example`.

## Notification Registry

Notifications work the same way. `NotificationRegistry::registerNotifier` binds
a name to a callback taking the event ID and title. `BuiltinNotifiers` registers
a simple `console` notifier that prints to stdout. When adding tasks via the CLI
you can choose both a notifier and an action.

## Calendar Integrations

External calendars can be kept in sync by attaching `CalendarApi` implementations to the model. The provided `GoogleCalendarApi` launches a small Python helper that talks to Google Calendar using a service account. Register the API after constructing your model:

```cpp
auto gcal = std::make_shared<GoogleCalendarApi>("service_account.json");
model.addCalendarApi(gcal);
```

Whenever events are added or removed, the Google calendar is updated automatically. Additional providers can subclass `CalendarApi` and call their own scripts.
The helper script is invoked with environment variables passed directly on the command line using the `env` tool. This avoids modifying the process environment so concurrent calendar updates cannot leak credentials between threads.

## API Security

The HTTP server can be secured via environment variables loaded from a `.env` file.
Set `API_KEY` to a secret string and provide it in the `Authorization` header for
all requests. Optional `ADMIN_API_KEY` may grant elevated privileges. Bind address
and port are configured with `HOST` and `PORT`. Rate limiting and CORS settings
are also configurable. See `.env.example` for details.
