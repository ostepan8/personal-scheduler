#include "Model.h"
#include <algorithm> // for std::sort, std::remove_if
#include <stdexcept> // for std::runtime_error
#include <random>
#include <sstream>

// Re‐sort internal list so that events are in ascending time order.
void Model::sortEvents()
{
    std::sort(events.begin(), events.end(),
              [](const Event &a, const Event &b)
              {
                  return a.getTime() < b.getTime();
              });
}

bool Model::eventExists(const std::string &id) const
{
    return std::any_of(events.begin(), events.end(), [&](const Event &ev) {
        return ev.getId() == id;
    });
}

std::string Model::generateUniqueId() const
{
    static std::mt19937_64 gen(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    std::string id;
    do
    {
        std::stringstream ss;
        ss << std::hex << dist(gen);
        id = ss.str();
    } while (eventExists(id));
    return id;
}

// Constructor: optionally load events from a database, otherwise use provided list.
Model::Model(std::vector<Event> init, IScheduleDatabase *db)
    : events(std::move(init)), db_(db)
{
    if (db_)
    {
        events = db_->getAllEvents();
    }
    sortEvents();
}

// ReadOnlyModel override: return up to maxOccurrences events occurring before endDate.
std::vector<Event>
Model::getEvents(int maxOccurrences,
                 std::chrono::system_clock::time_point endDate) const
{
    std::vector<Event> result;
    result.reserve(events.size());

    for (const auto &e : events)
    {
        // Stop once we exceed endDate
        if (e.getTime() > endDate)
        {
            break;
        }
        result.push_back(e);
        if (maxOccurrences > 0 && static_cast<int>(result.size()) >= maxOccurrences)
        {
            break;
        }
    }
    return result;
}

// ReadOnlyModel override: return the very next (earliest) event.
Event Model::getNextEvent() const
{
    if (events.empty())
    {
        throw std::runtime_error("No upcoming events.");
    }
    return events.front();
}

// ===== Mutation methods =====

bool Model::addEvent(Event &e)
{
    if (eventExists(e.getId()))
    {
        return false;
    }
    events.push_back(e);
    sortEvents();
    if (db_)
    {
        db_->addEvent(e);
    }
    return true;
}

bool Model::removeEvent(Event &e)
{
    return removeEvent(e.getId());
}

bool Model::removeEvent(const std::string &id)
{
    auto beforeSize = events.size();
    events.erase(std::remove_if(events.begin(), events.end(),
                                [&](const Event &ev)
                                {
                                    return ev.getId() == id;
                                }),
                 events.end());
    bool removed = events.size() < beforeSize;
    if (removed && db_)
    {
        db_->removeEvent(id);
    }
    return removed;
}
