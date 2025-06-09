#pragma once
#include "../model/Event.h"
#include <functional>
#include <chrono>
#include <memory>

class ScheduledTask : public Event {
    std::function<void()> notifyCb_;
    std::function<void()> actionCb_;
    std::chrono::system_clock::time_point notifyTime_;
    bool notified_ = false;

public:
    // Specify an absolute notification time
    ScheduledTask(const std::string &id,
                  const std::string &desc,
                  const std::string &title,
                  std::chrono::system_clock::time_point time,
                  std::chrono::system_clock::duration dur,
                  std::chrono::system_clock::time_point notifyTime,
                  std::function<void()> notifyCb,
                  std::function<void()> actionCb)
        : Event(id, desc, title, time, dur),
          notifyCb_(std::move(notifyCb)),
          actionCb_(std::move(actionCb)),
          notifyTime_(notifyTime) {}

    // Convenience constructor: notify a duration before execution
    ScheduledTask(const std::string &id,
                  const std::string &desc,
                  const std::string &title,
                  std::chrono::system_clock::time_point time,
                  std::chrono::system_clock::duration dur,
                  std::chrono::system_clock::duration notifyBefore,
                  std::function<void()> notifyCb,
                  std::function<void()> actionCb)
        : ScheduledTask(id, desc, title, time, dur,
                        time - notifyBefore,
                        std::move(notifyCb),
                        std::move(actionCb)) {}

    std::unique_ptr<Event> clone() const override { return std::make_unique<ScheduledTask>(*this); }

    void notify() override { if (notifyCb_) notifyCb_(); }

    void execute() override { if (actionCb_) actionCb_(); }

    std::chrono::system_clock::time_point getNotifyTime() const { return notifyTime_; }
    bool notified() const { return notified_; }
    void setNotified(bool v) { notified_ = v; }
};
