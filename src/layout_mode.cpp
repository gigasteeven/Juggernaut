// Vendored from XDBotFork layout_mode.cpp — adapted for Juggernaut
// Gate: layout transforms apply to PRIMARY only, never to shadow.

#include <Geode/Geode.hpp>
#include "layout_mode.hpp"

using namespace geode::prelude;

// ─── getModifiedString: strip deco from level string ───
// Vendored from XDBotFork LayoutMode::getModifiedString

std::string LayoutMode::mergeVector(std::vector<std::string> vec, std::string separator) {
    std::string result;
    for (size_t i = 0; i < vec.size(); i++) {
        result += vec[i];
        if (i < vec.size() - 1) result += separator;
    }
    return result;
}

static std::vector<std::string> splitString(const std::string& str, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(str);
    while (std::getline(stream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string LayoutMode::getModifiedString(std::string levelString) {
    // Decode the level string using Geode's ZipUtils
    // ZipUtils::decompressString(gd::string const&, bool, int) -> gd::string
    gd::string decoded = ZipUtils::decompressString(levelString, false, 0);
    if (decoded.empty()) return levelString;

    std::string decodedStr(decoded);

    // Split by semicolons to get objects
    auto parts = splitString(decodedStr, ';');
    if (parts.empty()) return levelString;

    // First part is the header — replace colors
    std::string header = parts[0];

    // Find and replace kS38 (color string)
    auto kS38pos = header.find("kS38,");
    if (kS38pos != std::string::npos) {
        auto endPos = header.find(",", kS38pos + 5);
        if (endPos == std::string::npos) endPos = header.length();
        header = header.substr(0, kS38pos + 5) + newColors + header.substr(endPos);
    }

    // Set background color to layout blue: kS1=40, kS2=125, kS3=255
    auto replaceOrAdd = [&](const std::string& key, const std::string& value) {
        auto pos = header.find(key + ",");
        if (pos != std::string::npos) {
            auto endPos = header.find(",", pos + key.length() + 1);
            if (endPos == std::string::npos) endPos = header.length();
            header = header.substr(0, pos + key.length() + 1) + value + header.substr(endPos);
        }
    };
    replaceOrAdd("kS1", "40");
    replaceOrAdd("kS2", "125");
    replaceOrAdd("kS3", "255");

    std::string result = header;

    // Process each object
    for (size_t i = 1; i < parts.size(); i++) {
        if (parts[i].empty()) continue;

        auto objProps = splitString(parts[i], ',');
        if (objProps.size() < 2) {
            result += ";" + parts[i];
            continue;
        }

        // Parse object properties into map
        std::map<int, std::string> propMap;
        for (size_t j = 0; j + 1 < objProps.size(); j += 2) {
            try {
                int key = std::stoi(objProps[j]);
                propMap[key] = objProps[j + 1];
            } catch (...) {}
        }

        int objectID = 0;
        if (propMap.count(1)) {
            try { objectID = std::stoi(propMap[1]); } catch (...) {}
        }

        // Skip excluded triggers entirely
        if (excludedTriggerIDs.contains(objectID)) continue;

        // Skip decoration objects
        if (decoObjectIDs.contains(objectID)) continue;

        // For important triggers, keep only the specified properties
        if (importantTriggerIDs.count(objectID)) {
            auto& keepProps = importantTriggerIDs.at(objectID);
            std::string newObj;
            for (size_t j = 0; j + 1 < objProps.size(); j += 2) {
                try {
                    int key = std::stoi(objProps[j]);
                    // Always keep object ID (1), position (2,3), groups (57), etc.
                    if (key == 1 || key == 2 || key == 3 || key == 57 ||
                        std::find(keepProps.begin(), keepProps.end(), key) != keepProps.end()) {
                        if (!newObj.empty()) newObj += ",";
                        newObj += objProps[j] + "," + objProps[j + 1];
                    }
                } catch (...) {}
            }
            result += ";" + newObj;
            continue;
        }

        // Keep object as-is (solid blocks, hazards, portals, etc.)
        result += ";" + parts[i];
    }

    // Re-encode using Geode's ZipUtils
    // ZipUtils::compressString(gd::string const&, bool, int) -> gd::string
    gd::string compressed = ZipUtils::compressString(result, false, 0);
    if (compressed.empty()) return levelString;
    return std::string(compressed);
}
