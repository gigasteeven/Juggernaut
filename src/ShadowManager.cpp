#include "ShadowManager.hpp"

#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/binding/GameObject.hpp>

// GL readback for Spout. cocos2d-x declares these in its platform GL header, but
// that path is not stable across Geode SDK layouts, so we declare the minimal
// GL surface we use directly. OpenGL32 is provided by the OS on Windows and is
// already linked by the GD process.
#ifdef GEODE_IS_WINDOWS
extern "C" {
    __declspec(dllimport) void __stdcall glReadPixels(int x, int y, int width, int height,
                                                      unsigned int format, unsigned int type, void* data);
}
#define GL_RGBA          0x1908
#define GL_UNSIGNED_BYTE 0x1401
#endif

// ============================================================================
// Creation / teardown
// ============================================================================
void ShadowManager::create(GJGameLevel* level) {
    destroy();
    if (!level) return;

    auto* gm = GameManager::get();
    PlayLayer* primary = gm->m_playLayer; // remember PRIMARY for restore

    m_level = level;
    m_creatingShadow = true;

    PlayLayer* shadow = PlayLayer::create(level, false, false); // full level, all objects

    m_creatingShadow = false;
    gm->m_playLayer = primary; // singleton MUST point at PRIMARY

    if (!shadow) {
        log::error("[Shadow] PlayLayer::create returned nullptr for shadow");
        m_shadow = nullptr;
        m_level = nullptr;
        return;
    }

    shadow->retain(); // detached from scene: keep it alive
    m_shadow = shadow;

    configureOutput(
        Mod::get()->getSettingValue<int>("shadow_width"),
        Mod::get()->getSettingValue<int>("shadow_height"));

    log::info("[Shadow] created shadow PlayLayer {} for level '{}'",
              static_cast<void*>(shadow),
              level->m_levelName.empty() ? std::string("?") : std::string(level->m_levelName));
}

void ShadowManager::destroy() {
    if (m_shadow) {
        log::info("[Shadow] destroying shadow PlayLayer {}", static_cast<void*>(m_shadow));
        m_shadow->release();
        m_shadow = nullptr;
    }
    if (m_rt) {
        m_rt->release();
        m_rt = nullptr;
    }
    m_spout.close();
    m_level = nullptr;
    m_loggedDrift = false;
    m_paused = false;
}

void ShadowManager::configureOutput(int width, int height) {
    if (width <= 0 || height <= 0) return;
    bool sizeChanged = (width != m_width || height != m_height);

    if (sizeChanged || !m_rt) {
        if (m_rt) { m_rt->release(); m_rt = nullptr; }

        auto* rt = cocos2d::CCRenderTexture::create(width, height);
        rt->retain();
        m_rt = rt;
        m_width = width;
        m_height = height;

        m_spout.close();
        auto name = Mod::get()->getSettingValue<std::string>("spout_sender_name");
        if (!name.empty()) m_spout.open(name, width, height);

        m_readback.assign((size_t)width * height * 4, 0);

        m_overlay.setup(width, height);
    }
}

// ============================================================================
// Per-frame lockstep: input mirror + single-source-of-time update + render
// ============================================================================
static inline void setButton(PlayerObject* pl, int btn, bool held, bool wasHeld) {
    if (held == wasHeld) return;
    if (held) pl->pushButton(static_cast<PlayerButton>(btn));
    else      pl->releaseButton(static_cast<PlayerButton>(btn));
}

void ShadowManager::applyMirrorInput() {
    if (!m_shadow) return;
    auto* p = GameManager::get()->m_playLayer;
    if (!p) return;

    bool cur[6] = {
        p->m_player1->m_holdingButtons[1], // jump
        p->m_player1->m_holdingButtons[2], // left
        p->m_player1->m_holdingButtons[3], // right
        p->m_player2 ? p->m_player2->m_holdingButtons[1] : false,
        p->m_player2 ? p->m_player2->m_holdingButtons[2] : false,
        p->m_player2 ? p->m_player2->m_holdingButtons[3] : false,
    };

    for (int i = 0; i < 3; i++) setButton(m_shadow->m_player1, i + 1, cur[i], m_mirrorHeld[i]);
    if (m_shadow->m_player2)
        for (int i = 0; i < 3; i++) setButton(m_shadow->m_player2, i + 1, cur[3 + i], m_mirrorHeld[3 + i]);

    for (int i = 0; i < 6; i++) m_mirrorHeld[i] = cur[i];
}

void ShadowManager::onPrimaryPostUpdate(float dt) {
    if (!m_shadow) return;

    // Auto-unpause when PRIMARY resumes (no fragile resume hook needed):
    // postUpdate stops being driven while PRIMARY is paused, but to be safe we
    // also reconcile the paused flag with PRIMARY's own paused state each tick.
    auto* p = GameManager::get()->m_playLayer;
    if (p && !p->m_isPaused) m_paused = false;

    if (m_paused) return; // frozen while pause menu is up

    applyMirrorInput();

    // Stepping the shadow. GD's gameplay physics lives in
    // GJBaseGameLayer::update(float) as a fixed-step loop driven by dt. Because
    // it's virtual, calling update() on a PlayLayer* resolves to the gameplay
    // update, not the bare CCNode one. The shadow is NOT in the scene graph so
    // the engine's scheduler never ticks it -> we tick it here with PRIMARY's
    // exact dt (single source of time) so it runs the same N fixed steps as
    // PRIMARY and stays frame-locked.
    m_shadow->update(dt);

    checkSync();
    renderShadowToTexture();

    // Feed overlay counters (jump edge -> click).
    if (p && p->m_player1)
        m_overlay.sampleClicks(p->m_player1->m_holdingButtons[1], dt);
}

void ShadowManager::onPrimaryReset() {
    if (!m_shadow) return;
    m_shadow->resetLevel();
    m_loggedDrift = false;
    for (int i = 0; i < 6; i++) m_mirrorHeld[i] = false;
}

void ShadowManager::checkSync() {
    auto* p = GameManager::get()->m_playLayer;
    if (!p || !m_shadow) return;

    auto& pp = p->m_player1->m_obPosition;
    auto& sp = m_shadow->m_player1->m_obPosition;

    if (pp.x != sp.x || pp.y != sp.y) {
        if (!m_loggedDrift) {
            log::warn("[Shadow/SYNC] drift: primary=({:.2f},{:.2f}) shadow=({:.2f},{:.2f})",
                      pp.x, pp.y, sp.x, sp.y);
            m_loggedDrift = true;
        }
    } else if (m_loggedDrift) {
        log::info("[Shadow/SYNC] re-synced (diff = 0)");
        m_loggedDrift = false;
    }
}

// ============================================================================
// Render SHADOW into the offscreen texture, draw indicators on top, publish.
// ============================================================================
void ShadowManager::renderShadowToTexture() {
    if (!m_shadow || !m_rt) return;

    // Progress % from PRIMARY for the overlay.
    int progress = 0;
    auto* p = GameManager::get()->m_playLayer;
    if (p) {
        // m_attemptTime or distance-based; simplest robust proxy is 0..100 from
        // the player's x vs level length. Guard against division by zero.
        float len = p->m_level ? (float)p->m_level->m_levelLength : 0.f;
        if (len > 0.f && p->m_player1)
            progress = (int)(p->m_player1->m_obPosition.x / len * 100.f);
        if (progress < 0) progress = 0;
        if (progress > 100) progress = 100;
    }

    m_overlay.updateValues(progress, m_spout.isOpen(), !m_loggedDrift);

    m_rt->beginWithClear(0.f, 0.f, 0.f, 1.f);
    m_shadow->visit();
    m_overlay.visit(); // indicators drawn into the OBS-facing texture only
    if (m_spout.isOpen()) {
        glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, m_readback.data());
    }
    m_rt->end();

    if (m_spout.isOpen()) {
        m_spout.sendFrame(m_readback.data(), m_width, m_height);
    }
}
