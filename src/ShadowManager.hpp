#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>

using namespace geode::prelude;

// Owns the SHADOW (second, offscreen, detached) PlayLayer of the current level.
//
// SHADOW is a full level (original, unmodified level string -> all decorations),
// kept alive manually via retain()/release() because it is NOT added to any scene.
// It is never the game singleton (GameManager::m_playLayer), which always points
// at PRIMARY (the layer the player actually sees).
//
// Step 1 scope: just create + destroy it stably. No layout, no Spout, no input sync yet.
class ShadowManager {
    PlayLayer* m_shadow = nullptr;
    GJGameLevel* m_level = nullptr;

    // Re-entrancy guard: true only while we are *inside* PlayLayer::create for the
    // shadow, so our own PlayLayer hooks can tell this instance apart from PRIMARY.
    bool m_creatingShadow = false;

    ShadowManager() = default;

public:
    static ShadowManager& get() {
        static ShadowManager instance;
        return instance;
    }

    // True while the shadow is mid-construction. Checked from PlayLayer::init.
    bool isCreatingShadow() const { return m_creatingShadow; }

    PlayLayer* shadow() const { return m_shadow; }
    bool hasShadow() const { return m_shadow != nullptr; }

    // Builds the shadow for the given level. Safe to call repeatedly; destroys any
    // previous shadow first. Restores the game singleton to PRIMARY afterwards.
    void create(GJGameLevel* level);

    // Releases the shadow and clears state. Safe to call when there is none.
    void destroy();
};
