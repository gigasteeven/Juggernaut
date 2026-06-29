#pragma once
#include <Geode/Geode.hpp>

class PlayLayer;
class GJGameLevel;

class ShadowManager {
public:
    static ShadowManager& get();

    void createShadow(GJGameLevel* level);
    void destroyShadow();
    void stepShadow(float dt);
    void resetShadow();

private:
    ShadowManager() = default;
    ShadowManager(const ShadowManager&) = delete;
    ShadowManager& operator=(const ShadowManager&) = delete;
};