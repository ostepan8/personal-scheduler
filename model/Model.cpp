#include "Model.h"
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

// Constructor: optionally load events from a database
Model::Model(IScheduleDatabase *db)
    : db_(db)
{
    if (db_)
    {
        auto loaded = db_->getAllEvents();
        for (auto &e : loaded)
        {
            events.emplace(e->getTime(), std::move(e));
        }
    }
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
    return true;
}

bool Model::removeEvent(const Event &e)
{
    return removeEvent(e.getId());
}

bool Model::removeEvent(const std::string &id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto beforeSize = events.size();
    for (auto it = events.begin(); it != events.end(); )
    {
        if (it->second->getId() == id)
            it = events.erase(it);
        else
            ++it;
    }
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
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    tm_buf.tm_hour = 0;
    tm_buf.tm_min = 0;
    tm_buf.tm_sec = 0;
#if defined(_MSC_VER)
    time_t start_t = _mkgmtime(&tm_buf);
#else
    time_t start_t = timegm(&tm_buf);
#endif
    return std::chrono::system_clock::from_time_t(start_t);
}

std::vector<Event> Model::getEventsOnDay(std::chrono::system_clock::time_point day) const
{
    auto start = startOfDay(day);
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
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    int wday = tm_buf.tm_wday; // 0=Sunday
    int diff = (wday + 6) % 7; // days since Monday
    auto start = startOfDay(day) - std::chrono::hours(24 * diff);
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
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    tm_buf.tm_mday = 1;
    tm_buf.tm_hour = 0;
    tm_buf.tm_min = 0;
    tm_buf.tm_sec = 0;
#if defined(_MSC_VER)
    time_t start_t = _mkgmtime(&tm_buf);
#else
    time_t start_t = timegm(&tm_buf);
#endif
    auto start = std::chrono::system_clock::from_time_t(start_t);
    tm_buf.tm_mon += 1;
#if defined(_MSC_VER)
    time_t end_t = _mkgmtime(&tm_buf);
#else
    time_t end_t = timegm(&tm_buf);
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
