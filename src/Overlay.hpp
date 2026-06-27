#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// ============================================================================
// ShadowOverlay
// ----------------------------------------------------------------------------
// On-screen-style indicators (FPS / CPS / progress / spout status) drawn ON TOP
// of the shadow render pass, so they appear in the OBS Spout2 capture but NOT
// on the player's monitor (PRIMARY). This is the readme "индикаторы в OBS".
//
// It is a plain CCNode holding labels; it is never added to a scene. Each frame
// ShadowManager calls updateValues() then visit() inside the render-texture pass.
// ============================================================================
class ShadowOverlay {
    cocos2d::CCLabelBMFont* m_fps = nullptr;
    cocos2d::CCLabelBMFont* m_cps = nullptr;
    cocos2d::CCLabelBMFont* m_status = nullptr;

    int m_width = 1920;
    int m_height = 1080;

    // FPS rolling counter.
    float m_fpsTimer = 0.f;
    int m_fpsFrames = 0;
    int m_fps = 0;

    // CPS (clicks per second) rolling counter.
    float m_cpsTimer = 0.f;
    int m_cpsClicks = 0;
    int m_cps = 0;

    // Edge detection: count a click when player1 jump goes false->true.
    bool m_lastJump = false;

public:
    ShadowOverlay() = default;

    // (Re)build labels for a given output size. Lazily created.
    void setup(int width, int height);

    // Count a click whenever PRIMARY p1 jump transitions to held.
    void sampleClicks(bool jumpHeld, float dt);

    // Refresh label strings from the latest counters.
    void updateValues(int progressPercent, bool spoutOpen, bool synced);

    void visit() {
        if (m_fps) m_fps->visit();
        if (m_cps) m_cps->visit();
        if (m_status) m_status->visit();
    }
};
