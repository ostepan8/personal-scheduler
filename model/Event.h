#pragma once

#include <string>
#include <chrono>
#include <memory>

class Event
{
protected:
    std::string id;
    std::string description;
    std::string title;
    // Stored in UTC regardless of the user's local timezone
    std::chrono::system_clock::time_point timeUtc;
    std::chrono::system_clock::duration duration;
    bool recurringFlag;
    std::string category; // New member for category support

public:
    // Updated constructor with optional category parameter
    Event(const std::string &id,
          const std::string &desc,
          const std::string &title,
          std::chrono::system_clock::time_point time,
          std::chrono::system_clock::duration duration,
          const std::string &category = "")
        : id(id),
          description(desc),
          title(title),
          timeUtc(time),
          duration(duration),
          recurringFlag(false),
          category(category) {}

    virtual ~Event() = default;

    // Clone method - derived classes should override to preserve their specific data
    virtual std::unique_ptr<Event> clone() const
    {
        return std::make_unique<Event>(*this);
    }

    // Called some minutes before the scheduled time. Default does nothing.
    virtual void notify() {}

    // Called when the scheduled time arrives. Default does nothing.
    virtual void execute() {}

    // ===== Getter methods =====

    // Returned value is in UTC
    std::chrono::system_clock::time_point getTime() const { return timeUtc; }
    std::chrono::system_clock::duration getDuration() const { return duration; }
    std::string getId() const { return id; }
    std::string getDescription() const { return description; }
    std::string getTitle() const { return title; }
    bool isRecurring() const { return recurringFlag; }
    std::string getCategory() const { return category; }

    // ===== Setter methods for updates =====

    void setDescription(const std::string &desc) { description = desc; }
    void setTitle(const std::string &newTitle) { title = newTitle; }
    void setTime(std::chrono::system_clock::time_point newTime) { timeUtc = newTime; }
    void setDuration(std::chrono::system_clock::duration newDuration) { duration = newDuration; }
    void setCategory(const std::string &newCategory) { category = newCategory; }

    // ===== Comparison operators for sorting =====

    bool operator<(const Event &other) const
    {
        return timeUtc < other.timeUtc;
    }

    bool operator==(const Event &other) const
    {
        return id == other.id;
    }

protected:
    void setRecurring(bool r) { recurringFlag = r; }

    // Protected setter for ID - only derived classes should modify
    void setId(const std::string &newId) { id = newId; }
};