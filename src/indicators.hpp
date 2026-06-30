#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

/// Renders FPS / CPS / Progress indicators on top of shadow render texture.
class Indicators {
public:
    static Indicators& get();

    /// Called each frame to update internal counters.
    void update(float dt);

    /// Register a click (for CPS counter).
    void registerClick();

    /// Draw indicators onto the given render texture (after shadow visit).
    void drawOnto(cocos2d::CCRenderTexture* renderTex);

    /// Get current FPS.
    int getFPS() const { return m_fps; }

    /// Get current CPS.
    int getCPS() const { return m_cps; }

    /// Get current progress percentage.
    float getProgress() const { return m_progress; }

private:
    Indicators() = default;

    // FPS
    int m_fps = 0;
    float m_fpsAccum = 0.f;
    int m_fpsFrames = 0;

    // CPS
    int m_cps = 0;
    std::vector<float> m_clickTimes;

    // Progress
    float m_progress = 0.f;
};
