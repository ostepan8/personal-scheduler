// BuiltinActions.h
#pragma once

#include "ActionRegistry.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>

class BuiltinActions
{
public:
    // Base endpoint for protocols
    inline static constexpr char PROTOCOL_ENDPOINT[] =
        "http://0.0.0.0:8000/protocols/run";

    // Original payload (kept for backward compatibility)
    inline static constexpr char PROTOCOL_PAYLOAD[] = R"({
        "protocol_name": "Dim All Lights",
        "arguments": {}
    })";

    // A simple "hello" action
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

    // Generic function to call Jarvis API with custom payload
    static void callJarvisApiWithPayload(const std::string &payload)
    {
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
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
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

    // Call your Jarvis "protocols/run" endpoint with JSON payload
    static void callJarvisApi()
    {
        std::cout << "Calling Jarvis Protocols API…\n";
        callJarvisApiWithPayload(PROTOCOL_PAYLOAD);
    }

    // Turn on all lights
    static void lightsOn()
    {
        std::cout << "Turning on all lights...\n";

        std::string payload = R"({
            "protocol_name": "lights_on",
            "arguments": {}
        })";

        callJarvisApiWithPayload(payload);
    }

    // Turn off all lights
    static void lightsOff()
    {
        std::cout << "Turning off all lights...\n";

        std::string payload = R"({
            "protocol_name": "lights_off",
            "arguments": {}
        })";

        callJarvisApiWithPayload(payload);
    }

    // Set lights to red
    static void lightsRed()
    {
        std::cout << "Setting all lights to red...\n";
        setLightsColor("red");
    }

    // Set lights to blue
    static void lightsBlue()
    {
        std::cout << "Setting all lights to blue...\n";
        setLightsColor("blue");
    }

    // Set lights to green
    static void lightsGreen()
    {
        std::cout << "Setting all lights to green...\n";
        setLightsColor("green");
    }

    // Set lights to yellow
    static void lightsYellow()
    {
        std::cout << "Setting all lights to yellow...\n";
        setLightsColor("yellow");
    }

    // Set lights to white
    static void lightsWhite()
    {
        std::cout << "Setting all lights to white...\n";
        setLightsColor("white");
    }

    // Set lights to purple
    static void lightsPurple()
    {
        std::cout << "Setting all lights to purple...\n";
        setLightsColor("purple");
    }

    // Set lights to orange
    static void lightsOrange()
    {
        std::cout << "Setting all lights to orange...\n";
        setLightsColor("orange");
    }

    // Set lights to pink
    static void lightsPink()
    {
        std::cout << "Setting all lights to pink...\n";
        setLightsColor("pink");
    }

    // Generic function to set lights to any color
    static void setLightsColor(const std::string &color)
    {
        std::stringstream ss;
        ss << R"({
            "protocol_name": "Light Color Control",
            "arguments": {
                "color": ")"
           << color << R"("
            }
        })";

        callJarvisApiWithPayload(ss.str());
    }

    // Advanced function to set lights to custom color (for programmatic use)
    static void setLightsToColor(const std::string &color)
    {
        std::cout << "Setting all lights to " << color << "...\n";

        // Validate color choice
        std::string validColors[] = {"red", "blue", "green", "yellow", "white", "purple", "orange", "pink", "read"};
        bool isValid = false;

        for (const auto &validColor : validColors)
        {
            if (color == validColor)
            {
                isValid = true;
                break;
            }
        }

        if (!isValid)
        {
            std::cerr << "Invalid color: " << color << ". Valid colors are: red, blue, green, yellow, white, purple, orange, pink\n";
            return;
        }

        setLightsColor(color);
    }

    // Register all built-in actions
    static void registerAll()
    {
        // Original actions
        ActionRegistry::registerAction("hello", &BuiltinActions::hello);
        ActionRegistry::registerAction("fetchExample", &BuiltinActions::fetchExample);
        ActionRegistry::registerAction("callJarvisApi", &BuiltinActions::callJarvisApi);

        // Lighting control actions
        ActionRegistry::registerAction("lightsOn", &BuiltinActions::lightsOn);
        ActionRegistry::registerAction("lightsOff", &BuiltinActions::lightsOff);
        ActionRegistry::registerAction("lightsRed", &BuiltinActions::lightsRed);
        ActionRegistry::registerAction("lightsBlue", &BuiltinActions::lightsBlue);
        ActionRegistry::registerAction("lightsGreen", &BuiltinActions::lightsGreen);
        ActionRegistry::registerAction("lightsYellow", &BuiltinActions::lightsYellow);
        ActionRegistry::registerAction("lightsWhite", &BuiltinActions::lightsWhite);
        ActionRegistry::registerAction("lightsPurple", &BuiltinActions::lightsPurple);
        ActionRegistry::registerAction("lightsOrange", &BuiltinActions::lightsOrange);
        ActionRegistry::registerAction("lightsPink", &BuiltinActions::lightsPink);

        // Alternative registration names for easier access
        ActionRegistry::registerAction("lights_on", &BuiltinActions::lightsOn);
        ActionRegistry::registerAction("lights_off", &BuiltinActions::lightsOff);
        ActionRegistry::registerAction("lights_red", &BuiltinActions::lightsRed);
        ActionRegistry::registerAction("lights_blue", &BuiltinActions::lightsBlue);
        ActionRegistry::registerAction("lights_green", &BuiltinActions::lightsGreen);
        ActionRegistry::registerAction("lights_yellow", &BuiltinActions::lightsYellow);
        ActionRegistry::registerAction("lights_white", &BuiltinActions::lightsWhite);
        ActionRegistry::registerAction("lights_purple", &BuiltinActions::lightsPurple);
        ActionRegistry::registerAction("lights_orange", &BuiltinActions::lightsOrange);
        ActionRegistry::registerAction("lights_pink", &BuiltinActions::lightsPink);
    }

    // Minimal HTTP JSON POST helper (for wake calls etc.)
    static void httpPostJson(const std::string &url, const std::string &payload,
                             long connect_timeout_sec = 3, long total_timeout_sec = 5)
    {
        CURL *curl = curl_easy_init();
        if (!curl)
        {
            std::cerr << "Failed to init curl\n";
            return;
        }
        std::string response;
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout_sec);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, total_timeout_sec);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "httpPostJson failed: " << curl_easy_strerror(res) << "\n";
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
};
