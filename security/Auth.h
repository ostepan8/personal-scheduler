#pragma once
#include "../api/httplib.h"
#include <string>

// Simple API key authenticator. Provides read-only and admin keys.
class Auth {
public:
    Auth(const std::string &key, const std::string &adminKey = "");
    bool authorize(const httplib::Request &req) const;
private:
    std::string key_;
    std::string adminKey_;
};
