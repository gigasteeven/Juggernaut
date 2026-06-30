// Juggernaut — main.cpp
// Dual-view GD mod: layout on screen, full level + indicators via Spout2 for OBS.
// Architecture "Variant B": two PlayLayers in one process.

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelTools.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include "layout_mode.hpp"
#include "shadow_layer.hpp"
#include "audio_guard.hpp"
#include "indicators.hpp"
#include "spout_output.hpp"

using namespace geode::prelude;

// ═══════════════════════════════════════════════════════════════
// Global state
// ═══════════════════════════════════════════════════════════════

static bool g_layoutModeActive = false;
static bool g_modEnabled = false;

static bool isEnabled() {
    return Mod::get()->getSettingValue<bool>("enabled");
}

// ═══════════════════════════════════════════════════════════════
// LevelTools: bypass verifyLevelIntegrity for layout-modified strings
// ═══════════════════════════════════════════════════════════════

class $modify(JuggLevelTools, LevelTools) {
    static bool verifyLevelIntegrity(gd::string v1, int v2) {
        if (g_layoutModeActive) return true;
        return LevelTools::verifyLevelIntegrity(v1, v2);
    }
};

// ═══════════════════════════════════════════════════════════════
// PlayLayer: Layout gate + Shadow lifecycle + Sync
// ═══════════════════════════════════════════════════════════════

class $modify(JuggPlayLayer, PlayLayer) {
    struct Fields {
        std::string m_originalString;
        float m_lastDt = 0.f;
    };

    // ─── addObject: layout gate ───
    // Apply layout transforms to EVERY addObject call on PRIMARY only.
    // Shadow gets full objects (no layout).
    void addObject(GameObject* obj) {
        if (!g_layoutModeActive || this == ShadowLayer::get().getShadow()) {
            PlayLayer::addObject(obj);
            return;
        }

        // Skip excluded triggers
        if (excludedTriggerIDs.contains(obj->m_objectID)) return;

        PlayLayer::addObject(obj);

        // Layout visual overrides (from XDBot)
        obj->m_activeMainColorID = -1;
        obj->m_activeDetailColorID = -1;
        obj->m_detailUsesHSV = false;
        obj->m_baseUsesHSV = false;
        obj->m_hasNoGlow = true;
        obj->m_isHide = obj->m_objectID == 2065;
        obj->setOpacity(obj->m_objectID == 2065 ? 0 : 255);
        obj->setVisible(obj->m_objectID != 2065);
    }

    // ─── init: Create primary (layout) + shadow (full) ───
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        g_modEnabled = isEnabled();
        if (!g_modEnabled) {
            return PlayLayer::init(level, useReplay, dontCreateObjects);
        }

        g_layoutModeActive = true;

        // Save original level string
        m_fields->m_originalString = std::string(level->m_levelString);

        // Modify level string for layout mode (PRIMARY)
        level->m_levelString = LayoutMode::getModifiedString(level->m_levelString);

        bool result = PlayLayer::init(level, useReplay, dontCreateObjects);

        // Restore original string (needed for shadow to load full level)
        level->m_levelString = m_fields->m_originalString;

        if (!result) {
            g_layoutModeActive = false;
            return false;
        }

        // ── Initialize Spout ──
        SpoutOutput::get().init();

        // ── Create shadow PlayLayer with ORIGINAL (full) level string ──
        // Temporarily disable layout mode so shadow gets full objects
        g_layoutModeActive = false;
        ShadowLayer::get().createShadow(level, useReplay, dontCreateObjects);
        g_layoutModeActive = true;

        log::info("[Juggernaut] PlayLayer init complete. Layout=PRIMARY, Full=SHADOW");

        return true;
    }

    // ─── update: Step both, then render ───
    void update(float dt) {
        if (!g_modEnabled || !ShadowLayer::get().isActive()) {
            PlayLayer::update(dt);
            return;
        }

        m_fields->m_lastDt = dt;

        // Step PRIMARY
        PlayLayer::update(dt);

        // Step SHADOW with same dt (single source of time)
        ShadowLayer::get().stepShadow(dt);

        // Update indicators
        Indicators::get().update(dt);

        // Debug: compare player positions for drift detection
        auto primaryPlayer = this->m_player1;
        auto shadowPlayer = ShadowLayer::get().getShadow()->m_player1;
        if (primaryPlayer && shadowPlayer) {
            float drift = std::abs(primaryPlayer->getPositionX() - shadowPlayer->getPositionX());
            if (drift > 1.0f) {
                log::warn("[Juggernaut] DRIFT detected: {:.2f} (primary={:.2f}, shadow={:.2f})",
                    drift, primaryPlayer->getPositionX(), shadowPlayer->getPositionX());
            }
        }
    }

    // ─── postUpdate: Render shadow + send to Spout ───
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!g_modEnabled || !ShadowLayer::get().isActive()) return;

        // Render shadow into offscreen texture
        ShadowLayer::get().renderFrame();

        // Draw indicators on top
        // (indicators are drawn onto the render texture)

        // Send frame to Spout
        SpoutOutput::get().sendShadowFrame();
    }

    // ─── resetLevel: Reset shadow too ───
    void resetLevel() {
        PlayLayer::resetLevel();

        if (!g_modEnabled) return;

        ShadowLayer::get().resetShadow();

        log::info("[Juggernaut] Level reset — shadow synced");
    }

    // ─── onQuit: Destroy shadow + release Spout ───
    void onQuit() {
        if (g_modEnabled) {
            ShadowLayer::get().destroyShadow();
            SpoutOutput::get().shutdown();
            g_layoutModeActive = false;
            g_modEnabled = false;
        }
        PlayLayer::onQuit();
    }
};

// ═══════════════════════════════════════════════════════════════
// CCDirector: Screen mirror for menus/editor (non-PlayLayer)
// ═══════════════════════════════════════════════════════════════

class $modify(JuggDirector, cocos2d::CCDirector) {
    void drawScene() {
        cocos2d::CCDirector::drawScene();

        // If we're NOT in PlayLayer, mirror screen to Spout
        if (!isEnabled()) return;

        auto gm = GameManager::sharedState();
        if (!gm->m_playLayer && SpoutOutput::get().isActive()) {
            SpoutOutput::get().sendScreenFrame();
        }
    }
};

// ═══════════════════════════════════════════════════════════════
// PlayerObject: CPS tracking for indicators
// ═══════════════════════════════════════════════════════════════

class $modify(JuggPlayerObject, PlayerObject) {
    void pushButton(PlayerButton btn) {
        PlayerObject::pushButton(btn);
        if (g_modEnabled) {
            Indicators::get().registerClick();
        }
    }
};

// ═══════════════════════════════════════════════════════════════
// Mod lifecycle
// ═══════════════════════════════════════════════════════════════

$on_mod(Loaded) {
    log::info("[Juggernaut] Mod loaded — dual-view system ready");
    // Spout is initialized lazily when entering a level
}
