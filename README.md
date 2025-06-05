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
