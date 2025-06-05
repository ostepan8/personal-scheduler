#pragma once

using namespace std;
#include <string>
#include <chrono>
class Event
{
protected:
    string id;
    string description;
    string title;
    // Stored in UTC regardless of the user's local timezone
    chrono::system_clock::time_point timeUtc;
    chrono::system_clock::duration duration;

public:
    Event(const string &id, const string &desc, const string &title, chrono::system_clock::time_point time,
          chrono::system_clock::duration duration)
        : id(id), description(desc), title(title), timeUtc(time), duration(duration) {}
    // Returned value is in UTC
    chrono::system_clock::time_point getTime() const { return timeUtc; }
    chrono::system_clock::duration getDuration() const { return duration; }
    string getId() const { return id; }
    string getDescription() const { return description; }
    string getTitle() const { return title; }
};