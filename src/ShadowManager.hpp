#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/binding/GameObject.hpp>

#include "Spout.hpp"
#include "Overlay.hpp"

using namespace geode::prelude;

// ============================================================================
// ShadowManager
// ----------------------------------------------------------------------------
// Owns the SHADOW PlayLayer: a second, detached instance of the current level
// rendered with FULL decorations into an offscreen render texture, then pushed
// to OBS via Spout2 (or shown as an on-screen preview when Spout is disabled).
//
// PRIMARY  = the PlayLayer the player sees (singleton GameManager::m_playLayer).
// SHADOW   = our second PlayLayer. NEVER the singleton. Retained manually.
//
// Sync model (see readme "СИНХРОНИЗАЦИЯ"):
//   - Single source of time: every frame, PRIMARY's postUpdate hands the exact
//     same dt to SHADOW and mirrors the current input state.
//   - SHADOW has NO schedule/accumulator of its own.
//   - Player physics depends only on solid/hazard (identical in both layers),
//     so determinism holds and the per-frame position diff is exactly 0.
// ============================================================================
class ShadowManager {
    PlayLayer* m_shadow = nullptr;
    GJGameLevel* m_level = nullptr;
    bool m_creatingShadow = false;
    bool m_paused = false;

    // Offscreen render target the shadow is drawn into.
    cocos2d::CCRenderTexture* m_rt = nullptr;
    int m_width = 1920;
    int m_height = 1080;

    // Persistent RGBA8 readback buffer (reused every frame -> no per-frame alloc).
    std::vector<unsigned char> m_readback;

    // Last input snapshot mirrored into the shadow (p1 + p2: jump/left/right).
    bool m_mirrorHeld[6] = {false, false, false, false, false, false};

    // Debug: per-frame position diff between PRIMARY p1 and SHADOW p1.
    bool m_loggedDrift = false;

    SpoutSender m_spout;
    ShadowOverlay m_overlay;

    ShadowManager() = default;

    void applyMirrorInput();
    void renderShadowToTexture();
    void checkSync();

public:
    static ShadowManager& get() {
        static ShadowManager instance;
        return instance;
    }

    bool isCreatingShadow() const { return m_creatingShadow; }
    PlayLayer* shadow() const { return m_shadow; }
    bool hasShadow() const { return m_shadow != nullptr; }
    bool isPaused() const { return m_paused; }

    void setPaused(bool paused) { m_paused = paused; }

    // Build the shadow + render texture for `level`.
    void create(GJGameLevel* level);

    // Release shadow + texture + spout.
    void destroy();

    // Called from PRIMARY's postUpdate every frame with PRIMARY's dt.
    // Steps the shadow in lockstep, mirrors input, renders to texture, pushes spout.
    void onPrimaryPostUpdate(float dt);

    // Called from PRIMARY's resetLevel (death/restart): reset shadow in lockstep.
    void onPrimaryReset();

    // Live reconfigure of output size (from settings menu).
    void configureOutput(int width, int height);
};
