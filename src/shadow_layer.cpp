// ShadowLayer: offscreen PlayLayer for Spout2 full-level output.
// Architecture "Variant B": two PlayLayers in one process.

#include "shadow_layer.hpp"
#include "audio_guard.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

ShadowLayer& ShadowLayer::get() {
    static ShadowLayer instance;
    return instance;
}

void ShadowLayer::createShadow(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
    if (m_shadow) destroyShadow();

    auto gm = GameManager::sharedState();
    auto primaryLayer = gm->m_playLayer;

    // Get screen size for offscreen render
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    m_width = static_cast<int>(winSize.width);
    m_height = static_cast<int>(winSize.height);

    // Create the offscreen render texture
    m_renderTex = CCRenderTexture::create(m_width, m_height);
    if (!m_renderTex) {
        log::error("[Juggernaut] Failed to create CCRenderTexture {}x{}", m_width, m_height);
        return;
    }
    m_renderTex->retain();

    // ─── Create shadow PlayLayer ───
    // Audio guard: suppress any audio calls during shadow creation
    AudioGuard::get().setShadowStepping(true);

    m_shadow = PlayLayer::create(level, useReplay, dontCreateObjects);

    AudioGuard::get().setShadowStepping(false);

    if (!m_shadow) {
        log::error("[Juggernaut] Failed to create shadow PlayLayer");
        m_renderTex->release();
        m_renderTex = nullptr;
        return;
    }

    // CRITICAL: restore singleton back to primary
    gm->m_playLayer = primaryLayer;

    // Retain shadow so it doesn't get autoreleased
    m_shadow->retain();

    // Shadow must receive onEnter to be alive, but unscheduleUpdate
    // so the global scheduler does NOT tick it (would cause 2x speed).
    m_shadow->onEnter();
    m_shadow->unscheduleUpdate();

    log::info("[Juggernaut] Shadow PlayLayer created ({}x{})", m_width, m_height);
}

void ShadowLayer::destroyShadow() {
    if (m_shadow) {
        m_shadow->onExit();
        m_shadow->release();
        m_shadow = nullptr;
    }
    if (m_renderTex) {
        m_renderTex->release();
        m_renderTex = nullptr;
    }
    log::info("[Juggernaut] Shadow PlayLayer destroyed");
}

void ShadowLayer::stepShadow(float dt) {
    if (!m_shadow) return;

    m_stepping = true;
    AudioGuard::get().setShadowStepping(true);

    // Step shadow with same dt as primary — single source of time
    m_shadow->update(dt);

    AudioGuard::get().setShadowStepping(false);
    m_stepping = false;
}

void ShadowLayer::resetShadow() {
    if (!m_shadow) return;

    m_stepping = true;
    AudioGuard::get().setShadowStepping(true);

    m_shadow->resetLevel();

    AudioGuard::get().setShadowStepping(false);
    m_stepping = false;
}

cocos2d::CCTexture2D* ShadowLayer::renderFrame() {
    if (!m_shadow || !m_renderTex) return nullptr;

    m_renderTex->beginWithClear(0, 0, 0, 1);
    m_shadow->visit();
    m_renderTex->end();

    // In cocos2d-x v2 (GD), rendering is immediate-mode.
    // The CCRenderTexture::end() already flushes the GL commands.
    // No explicit renderer->render() needed (that's cocos2d v3+).

    return m_renderTex->getSprite()->getTexture();
}

GLuint ShadowLayer::getFboID() const {
    if (!m_renderTex) return 0;
    // Return the GL texture name from the render texture's sprite
    auto sprite = m_renderTex->getSprite();
    if (!sprite) return 0;
    auto tex = sprite->getTexture();
    if (!tex) return 0;
    return tex->getName();
}
