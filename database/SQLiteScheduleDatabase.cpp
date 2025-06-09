#include "SQLiteScheduleDatabase.h"
#include "../model/OneTimeEvent.h"
#include "../model/RecurringEvent.h"
#include "../model/recurrence/DailyRecurrence.h"
#include "../model/recurrence/WeeklyRecurrence.h"
#include "../model/recurrence/MonthlyRecurrence.h"
#include "../model/recurrence/YearlyRecurrence.h"
#include "../utils/WeekDay.h"
#include "nlohmann/json.hpp"
#include <stdexcept>

SQLiteScheduleDatabase::SQLiteScheduleDatabase(const std::string &path)
    : db_(nullptr, sqlite3_close)
{
    sqlite3 *raw = nullptr;
    if (sqlite3_open(path.c_str(), &raw) != SQLITE_OK)
    {
        throw std::runtime_error("Failed to open database");
    }
    db_.reset(raw);

    const char *createSql =
        "CREATE TABLE IF NOT EXISTS events ("
        "id TEXT PRIMARY KEY,"
        "description TEXT,"
        "title TEXT,"
        "time INTEGER,"
        "duration INTEGER,"
        "recurrence TEXT);";
    char *errMsg = nullptr;
    if (sqlite3_exec(db_.get(), createSql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::string msg = errMsg ? errMsg : "error creating table";
        sqlite3_free(errMsg);
        throw std::runtime_error(msg);
    }
}

bool SQLiteScheduleDatabase::addEvent(const Event &e)
{
    const char *sql =
        "INSERT OR REPLACE INTO events (id, description, title, time, duration, recurrence) "
        "VALUES (?,?,?,?,?,?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, e.getId().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, e.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, e.getTitle().c_str(), -1, SQLITE_TRANSIENT);
    long long timeSec = std::chrono::duration_cast<std::chrono::seconds>(
                            e.getTime().time_since_epoch())
                            .count();
    sqlite3_bind_int64(stmt, 4, timeSec);
    long long durSec =
        std::chrono::duration_cast<std::chrono::seconds>(e.getDuration()).count();
    sqlite3_bind_int64(stmt, 5, durSec);
    std::string recJson;
    if (e.isRecurring())
    {
        const auto *re = dynamic_cast<const RecurringEvent *>(&e);
        if (re)
        {
            nlohmann::json j;
            auto pat = re->getRecurrencePattern();
            if (auto dr = dynamic_cast<DailyRecurrence *>(pat.get()))
            {
                j["type"] = "daily";
                j["interval"] = dr->getInterval();
                j["max"] = dr->getMaxOccurrences();
                auto endSec = std::chrono::duration_cast<std::chrono::seconds>(dr->getEndDate().time_since_epoch()).count();
                j["end"] = endSec;
            }
            else if (auto wr = dynamic_cast<WeeklyRecurrence *>(pat.get()))
            {
                j["type"] = "weekly";
                j["interval"] = wr->getInterval();
                std::vector<int> days;
                for (auto d : wr->getDaysOfWeek())
                    days.push_back(static_cast<int>(d));
                j["days"] = days;
                j["max"] = wr->getMaxOccurrences();
                auto endSec = std::chrono::duration_cast<std::chrono::seconds>(wr->getEndDate().time_since_epoch()).count();
                j["end"] = endSec;
            }
            else if (auto mr = dynamic_cast<MonthlyRecurrence *>(pat.get()))
            {
                j["type"] = "monthly";
                j["interval"] = mr->getInterval();
                j["max"] = mr->getMaxOccurrences();
                auto endSec = std::chrono::duration_cast<std::chrono::seconds>(mr->getEndDate().time_since_epoch()).count();
                j["end"] = endSec;
            }
            else if (auto yr = dynamic_cast<YearlyRecurrence *>(pat.get()))
            {
                j["type"] = "yearly";
                j["interval"] = yr->getInterval();
                j["max"] = yr->getMaxOccurrences();
                auto endSec = std::chrono::duration_cast<std::chrono::seconds>(yr->getEndDate().time_since_epoch()).count();
                j["end"] = endSec;
            }
            recJson = j.dump();
        }
    }
    if (recJson.empty())
        sqlite3_bind_null(stmt, 6);
    else
        sqlite3_bind_text(stmt, 6, recJson.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool SQLiteScheduleDatabase::removeEvent(const std::string &id)
{
    const char *sql = "DELETE FROM events WHERE id = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool SQLiteScheduleDatabase::removeAllEvents()
{
    const char *sql = "DELETE FROM events;";
    char *errMsg = nullptr;
    if (sqlite3_exec(db_.get(), sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        if (errMsg)
            sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::vector<std::unique_ptr<Event>> SQLiteScheduleDatabase::getAllEvents() const
{
    const char *sql =
        "SELECT id, description, title, time, duration, recurrence FROM events ORDER BY time;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    std::vector<std::unique_ptr<Event>> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string desc = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string title = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        long long timeSec = sqlite3_column_int64(stmt, 3);
        long long durSec = sqlite3_column_int64(stmt, 4);
        const unsigned char *recText = sqlite3_column_text(stmt, 5);
        auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(timeSec));
        auto dur = std::chrono::seconds(durSec);

        if (recText)
        {
            try
            {
                nlohmann::json j = nlohmann::json::parse(reinterpret_cast<const char *>(recText));
                std::shared_ptr<RecurrencePattern> pat;
                std::string type = j.value("type", "");
                if (type == "daily")
                {
                    int interval = j.value("interval", 1);
                    int max = j.value("max", -1);
                    long long endSec = j.value("end", std::numeric_limits<long long>::max());
                    auto endTp = std::chrono::system_clock::time_point(std::chrono::seconds(endSec));
                    pat = std::make_shared<DailyRecurrence>(tp, interval, max, endTp);
                }
                else if (type == "weekly")
                {
                    int interval = j.value("interval", 1);
                    std::vector<int> daysVals = j.value("days", std::vector<int>{});
                    std::vector<Weekday> days;
                    for (int d : daysVals)
                        days.push_back(static_cast<Weekday>(d));
                    int max = j.value("max", -1);
                    long long endSec = j.value("end", std::numeric_limits<long long>::max());
                    auto endTp = std::chrono::system_clock::time_point(std::chrono::seconds(endSec));
                    pat = std::make_shared<WeeklyRecurrence>(tp, days, interval, max, endTp);
                }
                else if (type == "monthly")
                {
                    int interval = j.value("interval", 1);
                    int max = j.value("max", -1);
                    long long endSec = j.value("end", std::numeric_limits<long long>::max());
                    auto endTp = std::chrono::system_clock::time_point(std::chrono::seconds(endSec));
                    pat = std::make_shared<MonthlyRecurrence>(tp, interval, max, endTp);
                }
                else if (type == "yearly")
                {
                    int interval = j.value("interval", 1);
                    int max = j.value("max", -1);
                    long long endSec = j.value("end", std::numeric_limits<long long>::max());
                    auto endTp = std::chrono::system_clock::time_point(std::chrono::seconds(endSec));
                    pat = std::make_shared<YearlyRecurrence>(tp, interval, max, endTp);
                }

                if (pat)
                {
                    auto ev = std::make_unique<RecurringEvent>(id, desc, title, tp, dur, pat);
                    result.push_back(std::move(ev));
                    continue;
                }
            }
            catch (...)
            {
                // fall back to one-time event
            }
        }
        auto ev = std::make_unique<OneTimeEvent>(id, desc, title, tp, dur);
        result.push_back(std::move(ev));
    }
    sqlite3_finalize(stmt);
    return result;
}
