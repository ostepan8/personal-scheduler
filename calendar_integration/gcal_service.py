#!/usr/bin/env python3
import os
import sys
from datetime import datetime
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request
from googleapiclient.discovery import build
import pickle

# If modifying these scopes, delete the file token.pickle.
SCOPES = ["https://www.googleapis.com/auth/calendar"]


def get_calendar_service(credentials_file):
    """Get authenticated Google Calendar service."""
    creds = None
    token_file = "calendar_integration/token.pickle"

    # Token file stores the user's access and refresh tokens
    if os.path.exists(token_file):
        with open(token_file, "rb") as token:
            creds = pickle.load(token)

    # If there are no (valid) credentials available, let the user log in
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(credentials_file, SCOPES)
            creds = flow.run_local_server(port=0)

        # Save the credentials for the next run
        with open(token_file, "wb") as token:
            pickle.dump(creds, token)

    return build("calendar", "v3", credentials=creds)


def main():
    # Get environment variables
    action = os.environ.get("GCAL_ACTION")
    creds_file = os.environ.get("GCAL_CREDS")
    calendar_id = os.environ.get("GCAL_CALENDAR_ID", "primary")

    if not action or not creds_file:
        print("Error: GCAL_ACTION and GCAL_CREDS must be set", file=sys.stderr)
        sys.exit(1)

    print(f"Action: {action}")
    print(f"Calendar ID: {calendar_id}")

    try:
        # Get authenticated service
        service = get_calendar_service(creds_file)

        if action == "add":
            # Get event details from environment
            event = {
                "summary": os.environ.get("GCAL_TITLE", "Untitled Event"),
                "description": os.environ.get("GCAL_DESC", ""),
                "start": {
                    "dateTime": os.environ["GCAL_START"],
                    "timeZone": os.environ.get("GCAL_TZ", "UTC"),
                },
                "end": {
                    "dateTime": os.environ["GCAL_END"],
                    "timeZone": os.environ.get("GCAL_TZ", "UTC"),
                },
            }

            # Add ID if provided (for consistency with C++ model)
            event_id = os.environ.get("GCAL_EVENT_ID")
            if event_id:
                event["id"] = event_id.replace("-", "").lower()[
                    :64
                ]  # Google Calendar ID constraints

            # Add recurrence if provided
            recurrence = os.environ.get("GCAL_RECURRENCE")
            if recurrence:
                event["recurrence"] = [recurrence]

            # Create the event
            created_event = (
                service.events().insert(calendarId=calendar_id, body=event).execute()
            )

            print(f"Event created: {created_event.get('htmlLink')}")
            print(f"Event ID: {created_event['id']}")

        elif action == "update":
            event_id = os.environ.get("GCAL_EVENT_ID")
            if not event_id:
                print("Error: GCAL_EVENT_ID required for update", file=sys.stderr)
                sys.exit(1)

            # Google Calendar IDs have specific format
            gcal_event_id = event_id.replace("-", "").lower()[:64]

            # Get the existing event first
            try:
                existing_event = (
                    service.events()
                    .get(calendarId=calendar_id, eventId=gcal_event_id)
                    .execute()
                )
            except Exception as e:
                print(
                    f"Error: Could not find event {gcal_event_id}: {e}", file=sys.stderr
                )
                sys.exit(1)

            # Update fields if provided
            if "GCAL_TITLE" in os.environ:
                existing_event["summary"] = os.environ["GCAL_TITLE"]
            if "GCAL_DESC" in os.environ:
                existing_event["description"] = os.environ["GCAL_DESC"]
            if "GCAL_START" in os.environ:
                existing_event["start"] = {
                    "dateTime": os.environ["GCAL_START"],
                    "timeZone": os.environ.get("GCAL_TZ", "UTC"),
                }
            if "GCAL_END" in os.environ:
                existing_event["end"] = {
                    "dateTime": os.environ["GCAL_END"],
                    "timeZone": os.environ.get("GCAL_TZ", "UTC"),
                }

            # Update the event
            updated_event = (
                service.events()
                .update(
                    calendarId=calendar_id, eventId=gcal_event_id, body=existing_event
                )
                .execute()
            )

            print(f"Event updated: {updated_event.get('htmlLink')}")

        elif action == "delete":
            event_id = os.environ.get("GCAL_EVENT_ID")
            if not event_id:
                print("Error: GCAL_EVENT_ID required for delete", file=sys.stderr)
                sys.exit(1)

            # Google Calendar IDs have specific format
            gcal_event_id = event_id.replace("-", "").lower()[:64]

            # Delete the event
            service.events().delete(
                calendarId=calendar_id, eventId=gcal_event_id
            ).execute()

            print(f"Event deleted: {event_id}")

        else:
            print(f"Error: Unknown action: {action}", file=sys.stderr)
            sys.exit(1)

    except Exception as e:
        print(f"Error: {type(e).__name__}: {e}", file=sys.stderr)
        import traceback

        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
