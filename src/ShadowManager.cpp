#include "ShadowManager.hpp"

#include <Geode/binding/GameManager.hpp>

void ShadowManager::create(GJGameLevel* level) {
    // Drop any previous shadow first (e.g. re-entering a level without a clean exit).
    destroy();

    if (!level) return;

    auto* gm = GameManager::get();

    // CRITICAL: GD assigns the active PlayLayer to the singleton during init.
    // Remember PRIMARY so we can restore it right after the shadow is built,
    // otherwise the game (and XDBot) would start treating the shadow as active.
    PlayLayer* primary = gm->m_playLayer;

    m_level = level;
    m_creatingShadow = true; // our PlayLayer::init will see this and tag the instance

    // Full level, normal replay rules, create ALL objects (full decorations).
    PlayLayer* shadow = PlayLayer::create(level, false, false);

    m_creatingShadow = false;

    // Restore singleton to PRIMARY no matter what happened inside create.
    gm->m_playLayer = primary;

    if (!shadow) {
        log::error("[Shadow] PlayLayer::create returned nullptr for shadow");
        m_shadow = nullptr;
        m_level = nullptr;
        return;
    }

    // create() returns an autoreleased object. Since the shadow is detached
    // (not in the scene graph), it would be freed at end of frame. Retain it.
    shadow->retain();
    m_shadow = shadow;

    log::info("[Shadow] created shadow PlayLayer {} for level '{}'",
              static_cast<void*>(shadow),
              level->m_levelName.value_or(std::string("?")));
}

void ShadowManager::destroy() {
    if (m_shadow) {
        log::info("[Shadow] destroying shadow PlayLayer {}",
                  static_cast<void*>(m_shadow));
        m_shadow->release();
        m_shadow = nullptr;
    }
    m_level = nullptr;
}
