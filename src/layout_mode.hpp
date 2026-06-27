#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>

// ============================================================================
// Vendored from XDBot (src/hacks/layout_mode.hpp).
// Layout string transformation: removes decorations, recolors, keeps only
// solid/hazard silhouettes + the trigger groups they depend on.
// Unchanged from upstream except: gated per-layer via LayoutGate (see .cpp).
// ============================================================================

struct LevelObject {
    std::vector<std::string> vecString;
    std::map<int, std::string> props;
    size_t hiddenIndex = 0;
    std::string ogString;
};

class LayoutMode {
public:
    static std::string getModifiedString(std::string levelString);
    static std::string mergeVector(std::vector<std::string> vec, std::string separator = ",");
};

// ---- Color overrides (layout = flat blue background palette) ----
const std::string newColors = "1_40_2_125_3_255_11_255_12_255_13_255_4_-1_6_1000_7_1_15_1_18_0_8_1|1_0_2_102_3_255_11_255_12_255_13_255_4_-1_6_1001_7_1_15_1_18_0_8_1|1_0_2_102_3_255_11_255_12_255_13_255_4_-1_6_1009_7_1_15_1_18_0_8_1|1_255_2_255_3_255_11_255_12_255_13_255_4_-1_6_1002_5_1_7_1_15_1_18_0_8_1|1_40_2_125_3_255_11_255_12_255_13_255_4_-1_6_1013_7_1_15_1_18_0_8_1|1_40_2_125_3_255_11_255_12_255_13_255_4_-1_6_1014_7_1_15_1_18_0_8_1|";

const std::unordered_set<int> robtopLevelIDs = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 3001, 5001, 5002, 5003, 5004 };

const std::unordered_set<int> excludedTriggerIDs = { 32, 33, 24, 23, 25, 26, 27, 28, 55, 56, 57, 58, 59, 22, 24, 27, 28, 29, 30, 56, 58, 59, 105, 899, 915, 1007, 1006, 2903, 2904, 2905, 2907, 2909, 2910, 2911, 2912, 2913, 2914, 2915, 2916, 2917, 2919, 2920, 2921, 2922, 2923, 2924, 3009,3010, 3029, 3030, 3031, 3606, 3612, 1612, 1613, 1818, 1819 };

const std::map<int, std::vector<int>> importantTriggerIDs = {
    {1346, {71, 51, 516, 517, 518, 519}},
    {2067, {71, 51}},
    {1347, {71, 51}},
    {3033, {71, 51}},
    {3016, {51, 71}},
    {3661, {51, 71}},
    {3006, {71, 51}},
    {3007, {71, 51}},
    {3008, {71, 51}},
    {1914, {71, 51}},
    {2062, {51, 71}},
    {901, {71, 395}},
    {3613, {71, 51}},
    {3022, {71, 51}},
    {3032, {71, 51}},
    {1811, {80, 51, 71}},
    {1611, {80, 51, 71}},
    {1814, {51}},
    {3660, {51}},
    {3027, {51}},
    {747, {51}},
    {2902, {51}},
    {2063, {51, 71 ,448}}
};

// NOTE: decoObjectIDs / solidObjectIDs are large; defined in layout_mode.cpp to
// keep this header browsable. They are byte-identical to XDBot upstream.
extern const std::unordered_set<int> decoObjectIDs;
extern const std::unordered_set<int> solidObjectIDs;
