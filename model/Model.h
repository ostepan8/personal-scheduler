#pragma once

#include <vector>
#include <chrono>
#include <string>
#include <memory>
#include "Event.h"
#include "ReadOnlyModel.h"
#include "../database/IScheduleDatabase.h"

/*
  Model extends ReadOnlyModel by adding mutators (addEvent, removeEvent).
  Internally it keeps a sorted vector of polymorphic Events using std::unique_ptr.
*/
class Model : public ReadOnlyModel
{
private:
    std::vector<std::unique_ptr<Event>> events;
    IScheduleDatabase *db_;

    // Check if an event ID already exists in the current list
    bool eventExists(const std::string &id) const;

    // Re‐sort the internal list whenever it changes.
    void sortEvents();

public:
    // Construct with an initial list of events and optional database.
    explicit Model(IScheduleDatabase *db = nullptr);

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

    // Generate a unique ID not currently in use
    std::string generateUniqueId() const;
};
