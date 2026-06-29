#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
using namespace geode::prelude;

// --- Совместимость с XDBot: Global с флагом layoutMode ---
class Global {
    Global() = default;
public:
    static auto& get() {
        static Global instance;
        return instance;
    }
    bool layoutMode = false;
};

// --- Совместимость с XDBot: Utils::splitByChar, numFromString ---
namespace Utils {
    inline std::vector<std::string> splitByChar(const std::string& s, char delim) {
        std::vector<std::string> out;
        std::string cur;
        for (char c : s) {
            if (c == delim) { out.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        out.push_back(cur);
        return out;
    }
}

// solidObjectIDs — extern, определяется в vendor/solid_ids.cpp
extern const std::unordered_set<int> solidObjectIDs;