#pragma once

#include <map>
#include <mutex>
#include <chrono>
#include <string>
#include <memory>
#include "Event.h"
#include "ReadOnlyModel.h"
#include "../database/IScheduleDatabase.h"
#include "../calendar/CalendarApi.h"
#include <vector>

/*
  Model extends ReadOnlyModel by adding mutators (addEvent, removeEvent).
  Internally it keeps events in a time-ordered container.  Access is protected
  by a mutex so multiple API threads can modify the schedule concurrently.
*/
class Model : public ReadOnlyModel
{
private:
    std::multimap<std::chrono::system_clock::time_point, std::unique_ptr<Event>> events;
    IScheduleDatabase *db_;
    mutable std::mutex mutex_;
    std::chrono::system_clock::time_point preloadEnd_;
    std::vector<std::shared_ptr<CalendarApi>> apis_;

    // Check if an event ID already exists in the current list
    bool eventExists(const std::string &id) const;


public:
    // Construct with an initial list of events and optional database.
    explicit Model(IScheduleDatabase *db = nullptr,
                   int preloadDaysAhead = -1);

    // ReadOnlyModel overrides (note the const):
    std::vector<Event>
    getEvents(int maxOccurrences,
              std::chrono::system_clock::time_point endDate) const override;

    Event getNextEvent() const override;
    // Return the next n upcoming event occurrences expanding recurring events.
    std::vector<Event> getNextNEvents(int n) const;

    // Additional query methods
    std::vector<Event> getEventsOnDay(std::chrono::system_clock::time_point day) const;
    std::vector<Event> getEventsInWeek(std::chrono::system_clock::time_point day) const;
    std::vector<Event> getEventsInMonth(std::chrono::system_clock::time_point day) const;

    // ====== Mutation methods ======

    // Add a new event. Returns true on success.
    bool addEvent(const Event &e);

    // Remove by full Event object (matches by ID internally). Returns true if removed.
    bool removeEvent(const Event &e);

    // Remove by ID. Returns true if at least one Event with that ID was erased.
    bool removeEvent(const std::string &id);

    // Remove all events from the model
    void removeAllEvents();

    // Remove events occurring on the given day. Returns number removed.
    int removeEventsOnDay(std::chrono::system_clock::time_point day);

    // Remove events in the week that contains the given day. Returns number removed.
    int removeEventsInWeek(std::chrono::system_clock::time_point day);

    // Remove all events strictly before the specified time. Returns number removed.
    int removeEventsBefore(std::chrono::system_clock::time_point time);

    // Generate a unique ID not currently in use
    std::string generateUniqueId() const;

    // Register an external calendar API to mirror changes
    void addCalendarApi(std::shared_ptr<CalendarApi> api);
};
