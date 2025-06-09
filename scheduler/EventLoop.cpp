#include "EventLoop.h"
#include <algorithm>

EventLoop::EventLoop(Model &m) : model_(m) {}

EventLoop::~EventLoop() { stop(); }

void EventLoop::start() {
    if (running_) return;
    running_ = true;
    worker_ = std::thread(&EventLoop::threadFunc, this);
}

void EventLoop::stop() {
    if (!running_) return;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        running_ = false;
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
}

void EventLoop::addTask(const std::shared_ptr<ScheduledTask> &task) {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(task);
    }
    cv_.notify_one();
    model_.addEvent(*task);
}

void EventLoop::threadFunc() {
    std::unique_lock<std::mutex> lock(mtx_);
    while (running_) {
        if (queue_.empty()) {
            cv_.wait(lock, [&]{ return !running_ || !queue_.empty(); });
            if (!running_) break;
        }

        auto next = queue_.top();
        auto now = std::chrono::system_clock::now();

        if (next->hasPendingNotifications() && now >= next->getNextNotifyTime()) {
            lock.unlock();
            next->notify();
            lock.lock();
            next->markNotificationSent();
            continue;
        }

        if (now >= next->getTime()) {
            queue_.pop();
            lock.unlock();
            next->execute();
            model_.removeEvent(next->getId());
            lock.lock();
            continue;
        }

        auto wake = next->getTime();
        if (next->hasPendingNotifications())
            wake = std::min(wake, next->getNextNotifyTime());
        cv_.wait_until(lock, wake);
    }
}
