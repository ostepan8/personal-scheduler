#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <algorithm>

namespace ActionRegistry {
    using Action = std::function<void()>;

    inline std::unordered_map<std::string, Action>& registry() {
        static std::unordered_map<std::string, Action> actions;
        return actions;
    }

    inline void registerAction(const std::string& name, Action action) {
        registry()[name] = std::move(action);
    }

    inline Action getAction(const std::string& name) {
        auto it = registry().find(name);
        if (it == registry().end()) return Action{};
        return it->second;
    }

    inline std::vector<std::string> availableActions() {
        std::vector<std::string> names;
        for (const auto& kv : registry()) names.push_back(kv.first);
        std::sort(names.begin(), names.end());
        return names;
    }
}
