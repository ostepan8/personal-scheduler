#include "OneTimeEvent.h"

OneTimeEvent::OneTimeEvent(const string &id, const string &desc, const string &title,
                           chrono::system_clock::time_point time, chrono::system_clock::duration duration)
    : Event(id, desc, title, time, duration)
{
}