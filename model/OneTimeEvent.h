#include "Event.h"
#include <chrono>
class OneTimeEvent : public Event
{
private:
public:
    OneTimeEvent(const string &id, const string &desc, const string &title, chrono::system_clock::time_point time,
                 chrono::system_clock::duration duration);
    std::unique_ptr<Event> clone() const override { return std::make_unique<OneTimeEvent>(*this); }
};