#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/binding/PauseLayer.hpp>

#include "ShadowManager.hpp"
#include "LayoutGate.hpp"

using namespace geode::prelude;

// ============================================================================
// Primary PlayLayer hooks (GD 2.2081 / Geode 5.7.1).
//
//   - init(...)              -> tag in-flight instance as shadow; gate layout
//   - setupHasCompleted()    -> spawn shadow for PRIMARY
//   - postUpdate(float dt)   -> lockstep step + render the shadow
//   - resetLevel()           -> reset shadow in lockstep on restart
//   - onExit()               -> teardown shadow on level exit
//
// Layout gating: PRIMARY only (via LayoutGate thread-local). SHADOW is never
// laid out -> full decorations.
//
// NOTE: we deliberately do NOT hook GJBaseGameLayer::update. The shadow is not
// in the scene graph, so the engine never schedules it; we drive it explicitly
// from postUpdate with PRIMARY's dt (single source of time).
// ============================================================================
class $modify(PlayLayer) {
    struct Fields {
        bool isShadow = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (ShadowManager::get().isCreatingShadow()) {
            m_fields->isShadow = true;
        }
        // Layout gate: the vendored XDBot layout hooks read this thread-local.
        // Active only for PRIMARY when the setting is on; never for SHADOW.
        LayoutGate::scope gate(!m_fields->isShadow &&
                               Mod::get()->getSettingValue<bool>("layout_on_primary"));
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        if (m_fields->isShadow) return;            // shadow must not recurse
        if (!Mod::get()->getSettingValue<bool>("enabled")) return;
        if (ShadowManager::get().hasShadow()) return;
        ShadowManager::get().create(this->m_level);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (m_fields->isShadow) return;            // shadow is driven by PRIMARY
        if (!ShadowManager::get().hasShadow()) return;
        ShadowManager::get().onPrimaryPostUpdate(dt);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (m_fields->isShadow) return;
        ShadowManager::get().onPrimaryReset();
    }

    void onExit() {
        PlayLayer::onExit();
        if (m_fields->isShadow) return;
        ShadowManager::get().destroy();
    }
};

// ============================================================================
// Pause: when PRIMARY is paused, freeze the shadow's simulation so neither
// drifts while the pause menu is up. We set paused=true when the PauseLayer
// appears, and onPrimaryPostUpdate clears it once PRIMARY is no longer paused
// (detected via PRIMARY's own m_isPaused field, no fragile resume hook).
//
// Audio note: the shadow is detached from the scene and never the active
// singleton, so GD does not drive its object SFX. Global music comes from the
// scene and is shared -> only PRIMARY is audible. No FMOD gate needed.
// ============================================================================
class $modify(PauseLayer) {
    void show(bool p0) {
        PauseLayer::show(p0);
        if (ShadowManager::get().hasShadow())
            ShadowManager::get().setPaused(true);
    }
};
