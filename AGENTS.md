# Agent Architecture

This document describes the evolving multi‑agent system that powers the scheduler.
It is written for both humans and agents that may need to reason about their roles
or extend the system.  The scheduler itself is a C++ application that exposes an
HTTP API.  Python agents communicate with the API and with each other to
interpret natural language, create or modify events, and optimise schedules.

## Overview

- **C++ Core** – Implements the actual scheduling engine.  It persists events to
  an SQLite database (`events.db`) and exposes a REST‐style API via `ApiServer`.
- **Python Agents** – Small autonomous workers that interpret commands,
  coordinate tasks and call the API.  They can be chained together to accomplish
  higher level goals (e.g. "plan my week").
- **ControllerAgent** – A supervisor agent that receives input, decides which
  specialised agents to invoke and resolves conflicts.

The high‑level flow:

```
User request → ControllerAgent → [ParserAgent, ScheduleAgent, OptimiserAgent…] →
C++ API → Database
```

## Agent Roles

### ControllerAgent

- **Purpose**: Entry point for new tasks.  Decides which specialist agents to
  invoke and maintains short‑term state of the conversation.
- **Capabilities**:
  - Route parsed intents to appropriate agents.
  - Detect conflicting updates and apply resolution rules.
  - Halt runaway loops by tracking depth/iterations.
- **Invocation**: Triggered whenever a new instruction arrives (CLI input,
  chat message or API call from an external service).

### ParserAgent

- **Purpose**: Convert natural language into structured intents the scheduler can
  understand.
- **Capabilities**:
  - Identify commands such as *add event*, *remove event*, *optimise*.
  - Extract times, durations and descriptions.
- **Invocation**: Called by ControllerAgent when raw text is received.

### ScheduleAgent

- **Purpose**: Create, modify or delete events by calling the C++ API.
- **Capabilities**:
  - `POST /events` to add one‑time events.
  - `DELETE /events/{id}` to remove events.
  - `GET /events` and related endpoints to query the schedule.
- **Invocation**: When ParserAgent identifies a scheduling intent or when
  OptimiserAgent proposes changes.

### OptimiserAgent

- **Purpose**: Analyse existing events and propose improved arrangements.
- **Capabilities**:
  - Query events through ScheduleAgent.
  - Apply heuristics or external AI models to minimise conflicts and maximise
    free time.
  - Return a list of modifications for ControllerAgent to approve.
- **Invocation**: Manually by the user (`"optimise my week"`) or on a schedule.

### MemoryAgent (optional)

- **Purpose**: Provide additional long‑term or shared memory beyond the
  scheduler database.
- **Capabilities**:
  - Store conversation summaries or notes in files or other databases.
  - Retrieve context for other agents.
- **Invocation**: On demand when agents need background information.

Agents may be extended or replaced; the names above serve as conventions for the
initial system.

## Communication

- Agents communicate via structured JSON messages.
- The ControllerAgent keeps a short‑term scratch pad (in memory) of the current
  conversation state and delegates tasks by sending messages to sub‑agents.
- The C++ API uses HTTP requests.  Example call to add an event:

```bash
curl -X POST http://localhost:8080/events \
  -H 'Content-Type: application/json' \
  -d '{"title":"Meet Bob","description":"Status call","time":"2024-05-01 15:00","duration":60}'
```

## Memory and State

- **Short‑term**: Each agent maintains in‑process memory during a single task.
  ControllerAgent also tracks the current goal and a history of delegated steps.
- **Long‑term**: Events are persisted to `events.db` via the C++ `Model` and
  `SQLiteScheduleDatabase`.  Agents should rely on API queries rather than
  direct DB access.
- **Logs**: Agents may write interaction logs or conversation summaries to disk
  for future reference and self‑improvement.

## Example Flows

### Add an Event
1. User says: *"Schedule a meeting with Bob tomorrow at 3pm for one hour."*
2. ControllerAgent sends text to ParserAgent.
3. ParserAgent returns an intent: `add_event` with time, duration and title.
4. ControllerAgent delegates to ScheduleAgent.
5. ScheduleAgent issues `POST /events` to the API.
6. API adds the event, updates the database and returns the new event details.

### Optimise a Week
1. User says: *"Optimise my week."*
2. ControllerAgent asks OptimiserAgent for suggestions.
3. OptimiserAgent queries upcoming events via ScheduleAgent.
4. It proposes moving or deleting certain events.
5. ControllerAgent checks for conflicts or missing approvals.
6. If approved, ControllerAgent instructs ScheduleAgent to apply changes.

## Adding a New Agent

1. **Name and file**: Create a Python module with a descriptive name
   (e.g. `ReminderAgent`).
2. **Role definition**: Document its purpose, input format and expected output.
3. **Registration**: Update ControllerAgent to recognise when this agent should
   run.  Typically this is a pattern match on the parsed intent.
4. **Communication**: Follow the JSON message schema used by existing agents.
5. **Testing**: Write unit tests or scripts that simulate messages to validate
   behaviour.

## Safety and Loop Prevention

- ControllerAgent tracks the depth and number of delegated calls.  If an agent
  exceeds a configurable limit (e.g. 10 nested tasks or 50 total actions), the
  controller aborts the operation and reports an error.
- Agents must always return a definitive success/failure response so the
  controller can decide next steps.
- Conflicting updates are resolved by querying the latest state from the API
  before committing any change.

## Example Agent Invocation Prompts

- *"Add lunch with Alice at noon"* → ParserAgent → ScheduleAgent
- *"What is my next event?"* → ParserAgent → ScheduleAgent (`GET /events/next`)
- *"Remove event ABC123"* → ParserAgent → ScheduleAgent (`DELETE /events/ABC123`)
- *"Optimise my month"* → ParserAgent → OptimiserAgent → ScheduleAgent

## Extending the System

- New agents should remain small and focused on a single responsibility.
- Prefer composing existing agents over duplicating logic.
- Agents may improve themselves by analysing logs or previous conversations and
  updating their own heuristics or prompt templates.
- When adding capabilities to the C++ API, update this document so agents know
  which endpoints are available.

---

This document should evolve along with the codebase.  Agents are encouraged to
read it before acting and to update it when their behaviours change.
