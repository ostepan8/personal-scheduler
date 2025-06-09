#include "Model.h"
#include "../calendar/CalendarApi.h"
#include "RecurringEvent.h"
#include <algorithm> // for std::sort, std::remove_if
#include <stdexcept> // for std::runtime_error
#include <random>
#include <sstream>
#include <memory>


bool Model::eventExists(const std::string &id) const
{
    return std::any_of(events.begin(), events.end(), [&](const auto &pair) {
        return pair.second->getId() == id;
    });
}

std::string Model::generateUniqueId() const
{
    static std::mt19937_64 gen(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    std::string id;
    std::lock_guard<std::mutex> lock(mutex_);
    do
    {
        std::stringstream ss;
        ss << std::hex << dist(gen);
        id = ss.str();
    } while (eventExists(id));
    return id;
}

// Constructor: optionally load events from a database. If preloadDaysAhead >= 0
// only events up to that many days in the future are loaded to reduce memory
// usage.
Model::Model(IScheduleDatabase *db, int preloadDaysAhead)
    : db_(db)
{
    if (preloadDaysAhead < 0)
        preloadEnd_ = std::chrono::system_clock::time_point::max();
    else
        preloadEnd_ = std::chrono::system_clock::now() +
                      std::chrono::hours(24 * preloadDaysAhead);

    if (db_)
    {
        auto loaded = db_->getAllEvents();
        for (auto &e : loaded)
        {
            if (preloadDaysAhead >= 0 && e->getTime() > preloadEnd_)
                continue;
            events.emplace(e->getTime(), std::move(e));
        }
    }
}

void Model::addCalendarApi(std::shared_ptr<CalendarApi> api)
{
    std::lock_guard<std::mutex> lock(mutex_);
    apis_.push_back(std::move(api));
}

// ReadOnlyModel override: return up to maxOccurrences events occurring before endDate.
std::vector<Event>
Model::getEvents(int maxOccurrences,
                 std::chrono::system_clock::time_point endDate) const
{
    std::vector<Event> result;
    std::lock_guard<std::mutex> lock(mutex_);
    result.reserve(events.size());

    for (const auto &kv : events)
    {
        const Event &e = *kv.second;
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

// Helper: return the next n upcoming occurrences across all events starting
// after the current time.
std::vector<Event> Model::getNextNEvents(int n) const
{
    std::vector<Event> occurrences;
    if (n <= 0)
        return occurrences;

    auto now = std::chrono::system_clock::now();
    auto start = now - std::chrono::seconds(1);

    std::lock_guard<std::mutex> lock(mutex_);
    if (events.empty())
        return occurrences;

    for (const auto &kv : events)
    {
        const Event &e = *kv.second;
        if (!e.isRecurring())
        {
            if (e.getTime() > now)
            {
                occurrences.push_back(e);
            }
        }
        else
        {
            const auto *re = dynamic_cast<const RecurringEvent *>(kv.second.get());
            if (!re)
                continue;

            auto times = re->getNextNOccurrences(start, n);
            for (auto t : times)
            {
                RecurringEvent occ(re->getId(), re->getDescription(), re->getTitle(),
                                   t, re->getDuration(), re->getRecurrencePattern());
                occurrences.push_back(occ);
            }
        }
    }

    std::sort(occurrences.begin(), occurrences.end(),
              [](const Event &a, const Event &b)
              { return a.getTime() < b.getTime(); });

    if (static_cast<int>(occurrences.size()) > n)
        occurrences.erase(occurrences.begin() + n, occurrences.end());
    return occurrences;
}

// ReadOnlyModel override: return the very next (earliest) event after now.
Event Model::getNextEvent() const
{
    auto list = getNextNEvents(1);
    if (list.empty())
        throw std::runtime_error("No upcoming events.");
    return list.front();
}

// ===== Mutation methods =====

bool Model::addEvent(const Event &e)
{
    std::vector<std::shared_ptr<CalendarApi>> apisCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (eventExists(e.getId()))
        {
            return false;
        }
        events.emplace(e.getTime(), e.clone());
        if (db_)
        {
            db_->addEvent(e);
        }
        apisCopy = apis_;
    }
    for (auto &api : apisCopy)
        api->addEvent(e);
    return true;
}

bool Model::removeEvent(const Event &e)
{
    return removeEvent(e.getId());
}

bool Model::removeEvent(const std::string &id)
{
    std::unique_ptr<Event> removed;
    std::vector<std::shared_ptr<CalendarApi>> apisCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = events.begin(); it != events.end(); )
        {
            if (it->second->getId() == id)
            {
                removed = std::move(it->second);
                it = events.erase(it);
            }
            else
            {
                ++it;
            }
        }
        if (removed && db_)
        {
            db_->removeEvent(id);
        }
        apisCopy = apis_;
    }
    if (removed)
        for (auto &api : apisCopy)
            api->deleteEvent(*removed);
    return static_cast<bool>(removed);
}

void Model::removeAllEvents()
{
    std::vector<std::unique_ptr<Event>> removed;
    std::vector<std::shared_ptr<CalendarApi>> apisCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &kv : events)
            removed.push_back(kv.second->clone());

        events.clear();
        if (db_)
        {
            db_->removeAllEvents();
        }
        apisCopy = apis_;
    }
    for (const auto &e : removed)
        for (auto &api : apisCopy)
            api->deleteEvent(*e);
}

// Return the start of the day in the user's local time zone
static std::chrono::system_clock::time_point startOfLocalDay(std::chrono::system_clock::time_point tp)
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
#if defined(_MSC_VER)
    time_t start_t = mktime(&tm_buf);
#else
    time_t start_t = mktime(&tm_buf);
#endif
    return std::chrono::system_clock::from_time_t(start_t);
}

std::vector<Event> Model::getEventsOnDay(std::chrono::system_clock::time_point day) const
{
    auto start = startOfLocalDay(day);
    auto end = start + std::chrono::hours(24);
    std::vector<Event> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &kv : events)
    {
        const Event &e = *kv.second;
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
    auto start = startOfLocalDay(day) - std::chrono::hours(24 * diff);
    auto end = start + std::chrono::hours(24 * 7);
    std::vector<Event> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &kv : events)
    {
        const Event &e = *kv.second;
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
#if defined(_MSC_VER)
    time_t start_t = mktime(&tm_buf);
#else
    time_t start_t = mktime(&tm_buf);
#endif
    auto start = std::chrono::system_clock::from_time_t(start_t);
    tm_buf.tm_mon += 1;
#if defined(_MSC_VER)
    time_t end_t = mktime(&tm_buf);
#else
    time_t end_t = mktime(&tm_buf);
#endif
    auto end = std::chrono::system_clock::from_time_t(end_t);

    std::vector<Event> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &kv : events)
    {
        const Event &e = *kv.second;
        if (e.getTime() < start)
            continue;
        if (e.getTime() >= end)
            break;
        result.push_back(e);
    }
    return result;
}

int Model::removeEventsOnDay(std::chrono::system_clock::time_point day)
{
    auto start = startOfLocalDay(day);
    auto end = start + std::chrono::hours(24);
    std::vector<std::string> removedIds;
    std::vector<std::unique_ptr<Event>> removedEvents;
    std::vector<std::shared_ptr<CalendarApi>> apisCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = events.begin(); it != events.end();)
        {
            auto t = it->second->getTime();
            if (t >= start && t < end)
            {
                removedIds.push_back(it->second->getId());
                removedEvents.push_back(it->second->clone());
                it = events.erase(it);
            }
            else
            {
                ++it;
            }
        }
        if (db_)
        {
            for (const auto &id : removedIds)
                db_->removeEvent(id);
        }
        apisCopy = apis_;
    }
    for (const auto &e : removedEvents)
        for (auto &api : apisCopy)
            api->deleteEvent(*e);
    return static_cast<int>(removedIds.size());
}

int Model::removeEventsInWeek(std::chrono::system_clock::time_point day)
{
    time_t t = std::chrono::system_clock::to_time_t(day);
    std::tm tm_buf;
#if defined(_MSC_VER)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    int wday = tm_buf.tm_wday;
    int diff = (wday + 6) % 7;
    auto start = startOfLocalDay(day) - std::chrono::hours(24 * diff);
    auto end = start + std::chrono::hours(24 * 7);
    std::vector<std::string> removedIds;
    std::vector<std::unique_ptr<Event>> removedEvents;
    std::vector<std::shared_ptr<CalendarApi>> apisCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = events.begin(); it != events.end();)
        {
            auto t2 = it->second->getTime();
            if (t2 >= start && t2 < end)
            {
                removedIds.push_back(it->second->getId());
                removedEvents.push_back(it->second->clone());
                it = events.erase(it);
            }
            else
            {
                ++it;
            }
        }
        if (db_)
        {
            for (const auto &id : removedIds)
                db_->removeEvent(id);
        }
        apisCopy = apis_;
    }
    for (const auto &e : removedEvents)
        for (auto &api : apisCopy)
            api->deleteEvent(*e);
    return static_cast<int>(removedIds.size());
}

int Model::removeEventsBefore(std::chrono::system_clock::time_point time)
{
    std::vector<std::string> removedIds;
    std::vector<std::unique_ptr<Event>> removedEvents;
    std::vector<std::shared_ptr<CalendarApi>> apisCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = events.begin(); it != events.end();)
        {
            if (it->second->getTime() < time)
            {
                removedIds.push_back(it->second->getId());
                removedEvents.push_back(it->second->clone());
                it = events.erase(it);
            }
            else
            {
                ++it;
            }
        }
        if (db_)
        {
            for (const auto &id : removedIds)
                db_->removeEvent(id);
        }
        apisCopy = apis_;
    }
    for (const auto &e : removedEvents)
        for (auto &api : apisCopy)
            api->deleteEvent(*e);
    return static_cast<int>(removedIds.size());
}
