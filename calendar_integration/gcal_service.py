from __future__ import annotations
import os
from .google_calendar_service import GoogleCalendarService
from .calendar_service import Event


def main():
    action = os.environ.get("GCAL_ACTION")
    creds = os.environ.get("GCAL_CREDS")
    calendar_id = os.environ.get("GCAL_CALENDAR_ID", "primary")
    if not action or not creds:
        raise SystemExit("GCAL_ACTION and GCAL_CREDS must be set")

    service = GoogleCalendarService(creds, calendar_id)

    if action in {"add", "update"}:
        title = os.environ["GCAL_TITLE"]
        start = os.environ["GCAL_START"]
        end = os.environ["GCAL_END"]
        desc = os.environ.get("GCAL_DESC", "")
        tz = os.environ.get("GCAL_TZ", "UTC")
        recurrence = os.environ.get("GCAL_RECURRENCE")
        event_id = os.environ.get("GCAL_EVENT_ID")
        event = Event(
            summary=title,
            start=start,
            end=end,
            description=desc,
            timezone=tz,
            id=event_id,
            recurrence=recurrence,
        )
        if action == "add":
            service.add_event(event)
        else:
            service.update_event(event)
    elif action == "delete":
        event_id = os.environ["GCAL_EVENT_ID"]
        service.delete_event(event_id)
    else:
        raise SystemExit(f"Unknown action: {action}")


if __name__ == "__main__":
    main()
