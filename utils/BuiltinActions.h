// BuiltinActions.h
#pragma once

#include "ActionRegistry.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <cstdlib>

class BuiltinActions
{
public:
    // Endpoint and payload as inline constexpr to avoid linker errors
    inline static constexpr char PROTOCOL_ENDPOINT[] =
        "http://0.0.0.0:8000/protocols/run";

    inline static constexpr char PROTOCOL_PAYLOAD[] = R"({
        "protocol_name": "Dim All Lights",
        "arguments": {}
    })";

    // A simple “hello” action
    static void hello()
    {
        std::cout << "Hello, world!\n";
    }

    // Fetch first 5 lines from example.com
    static void fetchExample()
    {
        std::cout << "Fetching example.com\n";
        std::system("curl -s https://example.com | head -n 5");
    }

    // Curl write callback to accumulate response
    static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
    {
        auto *response = static_cast<std::string *>(userdata);
        size_t total = size * nmemb;
        response->append(ptr, total);
        return total;
    }

    // Call your Jarvis “protocols/run” endpoint with JSON payload
    static void callJarvisApi()
    {
        std::cout << "Calling Jarvis Protocols API…\n";

        CURL *curl = curl_easy_init();
        if (!curl)
        {
            std::cerr << "Failed to init curl\n";
            return;
        }

        std::string response;
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, PROTOCOL_ENDPOINT);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, PROTOCOL_PAYLOAD);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: "
                      << curl_easy_strerror(res) << "\n";
        }
        else
        {
            std::cout << "Jarvis replied: " << response << "\n";
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    // Register all built-in actions
    static void registerAll()
    {
        ActionRegistry::registerAction("hello", &BuiltinActions::hello);
        ActionRegistry::registerAction("fetchExample", &BuiltinActions::fetchExample);
        ActionRegistry::registerAction("callJarvisApi", &BuiltinActions::callJarvisApi);
    }
};
