#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/PlayerObject.hpp>

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
// Pause: when the player resumes from the pause menu, unfreeze the shadow.
// onResume is the GD method called when the user hits "Resume" in PauseLayer
// (used the same way by XDBot: src/hacks/frame_stepper.cpp).
//
// We do NOT set paused=true on menu open: onPrimaryPostUpdate keeps running as
// long as PRIMARY's postUpdate fires, and PRIMARY's m_isPaused gates the shadow
// stepping inside ShadowManager. This resume hook just clears the flag early so
// there is no one-frame delay after unpausing.
//
// Audio note: the shadow is detached from the scene and never the active
// singleton, so GD does not drive its object SFX. Global music is scene-shared
// -> only PRIMARY is audible. No FMOD gate needed.
// ============================================================================
class $modify(PauseLayer) {
    void onResume(CCObject* sender) {
        PauseLayer::onResume(sender);
        if (ShadowManager::get().hasShadow())
            ShadowManager::get().setPaused(false);
    }
};
