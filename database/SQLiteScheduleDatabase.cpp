#include "SQLiteScheduleDatabase.h"
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
        "duration INTEGER);";
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
        "INSERT OR REPLACE INTO events (id, description, title, time, duration) "
        "VALUES (?,?,?,?,?);";
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

std::vector<Event> SQLiteScheduleDatabase::getAllEvents() const
{
    const char *sql =
        "SELECT id, description, title, time, duration FROM events ORDER BY time;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_.get(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    std::vector<Event> result;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string desc = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string title = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        long long timeSec = sqlite3_column_int64(stmt, 3);
        long long durSec = sqlite3_column_int64(stmt, 4);
        auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(timeSec));
        auto dur = std::chrono::seconds(durSec);
        result.emplace_back(id, desc, title, tp, dur);
    }
    sqlite3_finalize(stmt);
    return result;
}
