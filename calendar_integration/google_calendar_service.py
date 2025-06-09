from __future__ import annotations
from typing import Optional
from google.oauth2 import service_account
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError
from .calendar_service import CalendarService, Event

SCOPES = ["https://www.googleapis.com/auth/calendar"]

class GoogleCalendarService(CalendarService):
    """Google Calendar implementation of CalendarService."""

    def __init__(self, credentials_file: str, calendar_id: str = "primary"):
        creds = service_account.Credentials.from_service_account_file(
            credentials_file, scopes=SCOPES
        )
        self.service = build("calendar", "v3", credentials=creds)
        self.calendar_id = calendar_id

    def add_event(self, event: Event) -> str:
        body = {
            "summary": event.summary,
            "description": event.description,
            "start": {"dateTime": event.start, "timeZone": event.timezone},
            "end": {"dateTime": event.end, "timeZone": event.timezone},
        }
        if event.id:
            body["id"] = event.id

        created = (
            self.service.events()
            .insert(calendarId=self.calendar_id, body=body)
            .execute()
        )
        return created.get("id")

    def delete_event(self, event_id: str) -> None:
        try:
            self.service.events().delete(
                calendarId=self.calendar_id, eventId=event_id
            ).execute()
        except HttpError as exc:
            if exc.resp.status != 410:
                raise
