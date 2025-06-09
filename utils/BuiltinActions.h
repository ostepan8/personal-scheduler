#pragma once
#include "ActionRegistry.h"
#include <iostream>
#include <cstdlib>

class BuiltinActions {
public:
    static void hello() {
        std::cout << "Hello, world!\n";
    }

    static void fetchExample() {
        std::cout << "Fetching example.com" << std::endl;
        std::system("curl -s https://example.com | head -n 5");
    }

    static void registerAll() {
        ActionRegistry::registerAction("hello", &BuiltinActions::hello);
        ActionRegistry::registerAction("fetch_example", &BuiltinActions::fetchExample);
    }
};
