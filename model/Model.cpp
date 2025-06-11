#include "Model.h"
#include "../calendar/CalendarApi.h"
#include "RecurringEvent.h"
#include <algorithm>
#include <stdexcept>
#include <random>
#include <sstream>
#include <memory>
#include <cctype>
#include <regex>

bool Model::eventExists(const std::string &id) const
{
    return std::any_of(events.begin(), events.end(), [&](const auto &pair)
                       { return pair.second->getId() == id; });
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

        // Track category if event has one
        std::string category = e.getCategory();
        if (!category.empty())
        {
            categories_.insert(category);
        }

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
void Model::removeAllEvents()
{
    std::vector<std::unique_ptr<Event>> removed;
    std::vector<std::shared_ptr<CalendarApi>> apisCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &kv : events)
            removed.push_back(kv.second->clone());

        events.clear();
        categories_.clear();
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

// ====== NEW IMPLEMENTATIONS ======

// Search events by title or description
std::vector<Event> Model::searchEvents(const std::string &query, int maxResults) const
{
    std::vector<Event> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &[time, event] : events)
    {
        std::string title = event->getTitle();
        std::string desc = event->getDescription();
        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
        std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);

        if (title.find(lowerQuery) != std::string::npos ||
            desc.find(lowerQuery) != std::string::npos)
        {
            results.push_back(*event);
            if (maxResults > 0 && static_cast<int>(results.size()) >= maxResults)
            {
                break;
            }
        }
    }
    return results;
}

// Get events within a date range
std::vector<Event> Model::getEventsInRange(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const
{

    std::vector<Event> results;
    std::lock_guard<std::mutex> lock(mutex_);

    auto it_start = events.lower_bound(start);
    auto it_end = events.upper_bound(end);

    for (auto it = it_start; it != it_end; ++it)
    {
        results.push_back(*it->second);
    }
    return results;
}

// Get events by duration range
std::vector<Event> Model::getEventsByDuration(int minMinutes, int maxMinutes) const
{
    std::vector<Event> results;
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto &[time, event] : events)
    {
        auto durationMin = std::chrono::duration_cast<std::chrono::minutes>(event->getDuration()).count();
        if (durationMin >= minMinutes && durationMin <= maxMinutes)
        {
            results.push_back(*event);
        }
    }
    return results;
}

// Get events by category
std::vector<Event> Model::getEventsByCategory(const std::string &category) const
{
    std::vector<Event> results;
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto &[time, event] : events)
    {
        if (event->getCategory() == category)
        {
            results.push_back(*event);
        }
    }
    return results;
}

// Get all categories
std::set<std::string> Model::getCategories() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return categories_;
}

// Check for conflicts
std::vector<Event> Model::getConflicts(
    std::chrono::system_clock::time_point time,
    std::chrono::minutes duration) const
{

    std::vector<Event> conflicts;
    auto eventEnd = time + duration;

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &[existingTime, event] : events)
    {
        auto existingEnd = existingTime + event->getDuration();

        // Check if times overlap
        if (!(eventEnd <= existingTime || time >= existingEnd))
        {
            conflicts.push_back(*event);
        }
    }
    return conflicts;
}

// Find free time slots
std::vector<TimeSlot> Model::findFreeSlots(
    std::chrono::system_clock::time_point date,
    int startHour,
    int endHour,
    int minDurationMinutes) const
{

    std::vector<TimeSlot> freeSlots;

    // Get start and end of working hours
    auto dayStart = startOfLocalDay(date);
    auto workStart = dayStart + std::chrono::hours(startHour);
    auto workEnd = dayStart + std::chrono::hours(endHour);

    // Get all events for this day
    auto dayEvents = getEventsOnDay(date);

    // Sort events by time
    std::sort(dayEvents.begin(), dayEvents.end(),
              [](const Event &a, const Event &b)
              { return a.getTime() < b.getTime(); });

    // Find gaps
    auto currentTime = workStart;

    for (const auto &event : dayEvents)
    {
        if (event.getTime() > currentTime && event.getTime() < workEnd)
        {
            auto gap = std::chrono::duration_cast<std::chrono::minutes>(
                event.getTime() - currentTime);

            if (gap.count() >= minDurationMinutes)
            {
                TimeSlot slot;
                slot.start = currentTime;
                slot.end = event.getTime();
                slot.duration = gap;
                freeSlots.push_back(slot);
            }
        }

        auto eventEnd = event.getTime() + event.getDuration();
        if (eventEnd > currentTime)
        {
            currentTime = eventEnd;
        }
    }

    // Check for free time after last event
    if (currentTime < workEnd)
    {
        auto gap = std::chrono::duration_cast<std::chrono::minutes>(workEnd - currentTime);
        if (gap.count() >= minDurationMinutes)
        {
            TimeSlot slot;
            slot.start = currentTime;
            slot.end = workEnd;
            slot.duration = gap;
            freeSlots.push_back(slot);
        }
    }

    return freeSlots;
}

// Get next available slot
TimeSlot Model::findNextAvailableSlot(
    std::chrono::minutes duration,
    std::chrono::system_clock::time_point after,
    int startHour,
    int endHour) const
{

    auto currentDate = after;
    const int maxDaysToSearch = 30; // Don't search forever

    for (int days = 0; days < maxDaysToSearch; ++days)
    {
        auto slots = findFreeSlots(currentDate, startHour, endHour, duration.count());

        for (const auto &slot : slots)
        {
            if (slot.start >= after && slot.duration >= duration)
            {
                TimeSlot result;
                result.start = slot.start;
                result.end = slot.start + duration;
                result.duration = duration;
                return result;
            }
        }

        currentDate += std::chrono::hours(24);
    }

    // If no slot found, return a slot far in the future
    TimeSlot result;
    result.start = after + std::chrono::hours(24 * maxDaysToSearch);
    result.end = result.start + duration;
    result.duration = duration;
    return result;
}

// Get event statistics
EventStats Model::getEventStats(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const
{

    EventStats stats;
    stats.totalEvents = 0;
    stats.totalMinutes = 0;

    std::map<std::chrono::system_clock::time_point, int> eventsByDay;
    std::map<int, int> eventsByHour;

    auto events = getEventsInRange(start, end);

    for (const auto &event : events)
    {
        stats.totalEvents++;
        stats.totalMinutes += std::chrono::duration_cast<std::chrono::minutes>(
                                  event.getDuration())
                                  .count();

        // Count by category
        std::string category = event.getCategory();
        if (category.empty())
            category = "Uncategorized";
        stats.eventsByCategory[category]++;

        // Count by day
        auto dayStart = startOfLocalDay(event.getTime());
        eventsByDay[dayStart]++;

        // Count by hour
        time_t t = std::chrono::system_clock::to_time_t(event.getTime());
        std::tm tm_buf;
#if defined(_MSC_VER)
        localtime_s(&tm_buf, &t);
#else
        localtime_r(&t, &tm_buf);
#endif
        eventsByHour[tm_buf.tm_hour]++;
    }

    // Find busiest days (top 5)
    std::vector<std::pair<std::chrono::system_clock::time_point, int>> dayPairs(
        eventsByDay.begin(), eventsByDay.end());
    std::sort(dayPairs.begin(), dayPairs.end(),
              [](const auto &a, const auto &b)
              { return a.second > b.second; });

    for (size_t i = 0; i < std::min(size_t(5), dayPairs.size()); ++i)
    {
        stats.busiestDays.push_back(dayPairs[i]);
    }

    // Find busiest hours
    for (const auto &[hour, count] : eventsByHour)
    {
        stats.busiestHours.push_back({hour, count});
    }
    std::sort(stats.busiestHours.begin(), stats.busiestHours.end(),
              [](const auto &a, const auto &b)
              { return a.second > b.second; });

    return stats;
}

// Update an event
bool Model::updateEvent(const std::string &id, const Event &updatedEvent)
{
    std::vector<std::shared_ptr<CalendarApi>> apisCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Find and remove the old event
        std::unique_ptr<Event> oldEvent;
        for (auto it = events.begin(); it != events.end(); ++it)
        {
            if (it->second->getId() == id)
            {
                oldEvent = std::move(it->second);
                events.erase(it);
                break;
            }
        }

        if (!oldEvent)
        {
            return false;
        }

        // Add the updated event with the same ID
        auto newEvent = updatedEvent.clone();
        events.emplace(updatedEvent.getTime(), std::move(newEvent));

        // Update category tracking
        std::string newCategory = updatedEvent.getCategory();
        if (!newCategory.empty())
        {
            categories_.insert(newCategory);
        }

        if (db_)
        {
            // Note: You may need to implement updateEvent in IScheduleDatabase
            // For now, we'll remove and re-add
            db_->removeEvent(id);
            db_->addEvent(updatedEvent);
        }

        apisCopy = apis_;
    }

    // Notify APIs
    for (auto &api : apisCopy)
    {
        // Note: You may need to implement updateEvent in CalendarApi
        // For now, we'll delete and re-add
        api->deleteEvent(updatedEvent);
        api->addEvent(updatedEvent);
    }

    return true;
}

bool Model::updateEventFields(const std::string &id,
                              const std::unordered_map<std::string, std::string> &fields)
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto &[time, event] : events)
    {
        if (event->getId() == id)
        {
            // Update each field if present
            if (fields.count("title"))
            {
                event->setTitle(fields.at("title"));
            }
            if (fields.count("description"))
            {
                event->setDescription(fields.at("description"));
            }
            if (fields.count("category"))
            {
                event->setCategory(fields.at("category"));
                categories_.insert(fields.at("category"));
            }
            // Note: time and duration updates would require re-sorting in multimap
            // so those should use updateEvent() instead

            return true;
        }
    }
    return false;
}

// Get event by ID
std::unique_ptr<Event> Model::getEventById(const std::string &id) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto &[time, event] : events)
    {
        if (event->getId() == id)
        {
            return event->clone();
        }
    }
    return nullptr;
}

// Validate event time (no conflicts)
bool Model::validateEventTime(const Event &e) const
{
    auto conflicts = getConflicts(e.getTime(),
                                  std::chrono::duration_cast<std::chrono::minutes>(e.getDuration()));
    return conflicts.empty();
}

// Add multiple events
std::vector<bool> Model::addEvents(const std::vector<Event> &newEvents)
{
    std::vector<bool> results;
    results.reserve(newEvents.size());

    for (const auto &event : newEvents)
    {
        results.push_back(addEvent(event));
    }

    return results;
}

// Remove multiple events
int Model::removeEvents(const std::vector<std::string> &ids)
{
    int removed = 0;
    for (const auto &id : ids)
    {
        if (removeEvent(id))
        {
            removed++;
        }
    }
    return removed;
}

// Update multiple events
std::vector<bool> Model::updateEvents(
    const std::vector<std::pair<std::string, Event>> &updates)
{
    std::vector<bool> results;
    results.reserve(updates.size());

    for (const auto &[id, event] : updates)
    {
        results.push_back(updateEvent(id, event));
    }

    return results;
}

// Soft delete support
bool Model::removeEvent(const std::string &id, bool softDelete)
{
    if (softDelete)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Find and move to deleted events
        for (auto it = events.begin(); it != events.end(); ++it)
        {
            if (it->second->getId() == id)
            {
                deletedEvents.emplace(it->first, std::move(it->second));
                events.erase(it);
                return true;
            }
        }
        return false;
    }
    else
    {
        // Regular hard delete
        return removeEvent(id);
    }
}

// Get deleted events
std::vector<Event> Model::getDeletedEvents() const
{
    std::vector<Event> results;
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto &[time, event] : deletedEvents)
    {
        results.push_back(*event);
    }
    return results;
}

// Restore deleted event
bool Model::restoreEvent(const std::string &id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Find in deleted events
    for (auto it = deletedEvents.begin(); it != deletedEvents.end(); ++it)
    {
        if (it->second->getId() == id)
        {
            // Move back to active events
            events.emplace(it->first, std::move(it->second));
            deletedEvents.erase(it);
            return true;
        }
    }
    return false;
}