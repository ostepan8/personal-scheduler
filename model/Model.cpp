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

static std::chrono::system_clock::time_point startOfDay(std::chrono::system_clock::time_point tp)
{
    time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf;
#if defined(_MSC_VER)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    tm_buf.tm_hour = 0;
    tm_buf.tm_min = 0;
    tm_buf.tm_sec = 0;
    time_t start_t = std::mktime(&tm_buf);
    return std::chrono::system_clock::from_time_t(start_t);
}

std::vector<Event> Model::getEventsOnDay(std::chrono::system_clock::time_point day) const
{
    auto start = startOfDay(day);
    auto end = start + std::chrono::hours(24);
    std::vector<Event> result;
    for (const auto &e : events)
    {
        if (e.getTime() < start)
            continue;
        if (e.getTime() >= end)
            break;
        result.push_back(e);
    }
    return result;
}

std::vector<Event> Model::getEventsInWeek(std::chrono::system_clock::time_point day) const
{
    time_t t = std::chrono::system_clock::to_time_t(day);
    std::tm tm_buf;
#if defined(_MSC_VER)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    int wday = tm_buf.tm_wday; // 0=Sunday
    int diff = (wday + 6) % 7; // days since Monday
    auto start = startOfDay(day) - std::chrono::hours(24 * diff);
    auto end = start + std::chrono::hours(24 * 7);
    std::vector<Event> result;
    for (const auto &e : events)
    {
        if (e.getTime() < start)
            continue;
        if (e.getTime() >= end)
            break;
        result.push_back(e);
    }
    return result;
}

std::vector<Event> Model::getEventsInMonth(std::chrono::system_clock::time_point day) const
{
    time_t t = std::chrono::system_clock::to_time_t(day);
    std::tm tm_buf;
#if defined(_MSC_VER)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    tm_buf.tm_mday = 1;
    tm_buf.tm_hour = 0;
    tm_buf.tm_min = 0;
    tm_buf.tm_sec = 0;
    time_t start_t = std::mktime(&tm_buf);
    auto start = std::chrono::system_clock::from_time_t(start_t);
    tm_buf.tm_mon += 1;
    time_t end_t = std::mktime(&tm_buf);
    auto end = std::chrono::system_clock::from_time_t(end_t);

    std::vector<Event> result;
    for (const auto &e : events)
    {
        if (e.getTime() < start)
            continue;
        if (e.getTime() >= end)
            break;
        result.push_back(e);
    }
    return result;
}
