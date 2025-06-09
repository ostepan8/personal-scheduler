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

## Event Loop

The scheduler now includes an active `EventLoop` component. Tasks derived from
`Event` (for example `ScheduledTask`) can register notification and execution
callbacks. The loop runs in a background thread, dispatching notifications a
few minutes before each task executes and invoking the task's action at the
scheduled time.
