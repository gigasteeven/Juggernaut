#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/PlayerObject.hpp>

#include "ShadowManager.hpp"
#include "LayoutGate.hpp"
#include "layout_mode.hpp"

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
        bool applyLayout = !m_fields->isShadow &&
                           Mod::get()->getSettingValue<bool>("layout_on_primary");
        LayoutGate::scope gate(applyLayout);

        // CORE: apply layout to PRIMARY by transforming the level string.
        // We swap level->m_levelString to the layout version for the duration
        // of init (object parsing happens inside init), then restore the
        // original so the GJGameLevel object is never mutated persistently.
        // This mirrors XDBot's PlayLayer::init hook exactly.
        if (applyLayout) {
            gd::string original = level->m_levelString;
            std::string modified = LayoutMode::getModifiedString(original);
            level->m_levelString = modified;
            bool ok = PlayLayer::init(level, useReplay, dontCreateObjects);
            level->m_levelString = original; // always restore, even on failure
            return ok;
        }
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
// ============================================================================
class $modify(PauseLayer) {
    void onResume(CCObject* sender) {
        PauseLayer::onResume(sender);
        if (ShadowManager::get().hasShadow())
            ShadowManager::get().setPaused(false);
    }
};

// ============================================================================
// Audio gate: GD's music channel is a GLOBAL singleton in FMODAudioEngine.
// The shadow's gameplay loop (ticked by our manual update/resetLevel) also
// drives audio — startMusic / setMusicTimeMS / playEffect etc. If left
// unguarded, the shadow fights PRIMARY for the one music channel:
//   - wrong music offset when using a startpos (shadow re-seeks it)
//   - crash on rapid startpos switch (two layers racing the channel)
//
// Fix: drop EVERY audio call made while ShadowManager.isSteppingShadow() is
// true. Those calls originate ONLY from the shadow's update/reset, never from
// PRIMARY (PRIMARY's audio is driven by the engine scheduler, outside our
// stepping window). Net effect: only PRIMARY is audible.
// ============================================================================
class $modify(FMODAudioEngine) {
    void playEffect(gd::string path, float speed, float p2, float volume) {
        if (ShadowManager::get().isSteppingShadow()) return;
        FMODAudioEngine::playEffect(path, speed, p2, volume);
    }

    // Music seeking: the shadow's resetLevel (on death/restart/startpos switch)
    // calls setMusicTimeMS to re-seek the GLOBAL music channel to the startpos
    // offset. That overwrites the position PRIMARY set -> wrong music offset, and
    // when two layers race the channel on a rapid startpos switch -> crash.
    // Drop music control while the shadow is stepping.
    void setMusicTimeMS(int ms, bool p1, int channel) {
        if (ShadowManager::get().isSteppingShadow()) return;
        FMODAudioEngine::setMusicTimeMS(ms, p1, channel);
    }
};
