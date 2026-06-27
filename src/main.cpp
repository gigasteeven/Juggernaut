#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>

#include "ShadowManager.hpp"

using namespace geode::prelude;

// Step 1: stably create and destroy a SECOND (shadow) PlayLayer offscreen.
//
// Hooks (all GD 2.2081 / Geode 5.7.1):
//   - PlayLayer::init(level, useReplay, dontCreateObjects)
//       Used to tag the in-flight instance as the shadow while we are building it.
//   - PlayLayer::setupHasCompleted()
//       Fires once when PRIMARY finished loading -> spawn the shadow here.
//   - PlayLayer::onExit()
//       Fires when PRIMARY leaves the level (back to menu) -> tear the shadow down.
class $modify(PlayLayer) {
    // Per-instance flag. PRIMARY has isShadow=false; SHADOW has isShadow=true.
    // This is the foundation of the per-layer gating required later for layout.
    struct Fields {
        bool isShadow = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        // If ShadowManager is currently building us, we ARE the shadow.
        if (ShadowManager::get().isCreatingShadow()) {
            m_fields->isShadow = true;
        }
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        // The shadow must not recurse into creating another shadow, and only
        // PRIMARY is responsible for spawning it.
        if (m_fields->isShadow) return;
        if (!Mod::get()->getSettingValue<bool>("enabled")) return;
        if (ShadowManager::get().hasShadow()) return;

        ShadowManager::get().create(this->m_level);
    }

    void onExit() {
        PlayLayer::onExit();

        // The shadow is detached from the scene, so onExit should never fire on it;
        // guard anyway. When PRIMARY exits the level, drop the shadow.
        if (m_fields->isShadow) return;
        ShadowManager::get().destroy();
    }
};
