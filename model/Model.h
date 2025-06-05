#pragma once

#include <vector>
#include <chrono>
#include <string>
#include "Event.h"
#include "ReadOnlyModel.h"

/*
  Model extends ReadOnlyModel by adding mutators (addEvent, removeEvent).
  Internally it keeps a sorted vector<Event> and implements all queries.
*/
class Model : public ReadOnlyModel
{
private:
    std::vector<Event> events;

    // Re‐sort the internal list whenever it changes.
    void sortEvents();

public:
    // Construct with an initial list of events. They’re sorted right away.
    explicit Model(std::vector<Event> init);

    // ReadOnlyModel overrides (note the const):
    std::vector<Event>
    getEvents(int maxOccurrences,
              std::chrono::system_clock::time_point endDate) const override;

    Event getNextEvent() const override;

    // ====== Mutation methods ======

    // Add a new event. Returns true on success.
    bool addEvent(Event &e);

    // Remove by full Event object (matches by ID internally). Returns true if removed.
    bool removeEvent(Event &e);

    // Remove by ID. Returns true if at least one Event with that ID was erased.
    bool removeEvent(const std::string &id);
};
