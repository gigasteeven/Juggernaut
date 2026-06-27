#include "Overlay.hpp"

void ShadowOverlay::setup(int width, int height) {
    m_width = width;
    m_height = height;

    if (!m_fps) {
        m_fps = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        m_fps->retain();
        m_fps->setAnchorPoint({0.f, 1.f});
        m_fps->setScale(1.1f);
        m_fps->setColor({255, 255, 255});

        m_cps = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        m_cps->retain();
        m_cps->setAnchorPoint({0.f, 1.f});
        m_cps->setScale(1.1f);
        m_cps->setColor({255, 255, 255});

        m_status = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        m_status->retain();
        m_status->setAnchorPoint({0.f, 1.f});
        m_status->setScale(0.8f);
        m_status->setColor({180, 180, 180});
    }

    // Top-left stack, origin = top-left in the render-texture's coordinate space.
    // CCRenderTexture draws with (0,0) bottom-left; we place labels at top via y.
    const float margin = 12.f;
    m_fps->setPosition({margin, (float)m_height - margin});
    m_cps->setPosition({margin, (float)m_height - margin - 32.f});
    m_status->setPosition({margin, (float)m_height - margin - 64.f});
}

void ShadowOverlay::sampleClicks(bool jumpHeld, float dt) {
    if (jumpHeld && !m_lastJump) m_cpsClicks++;
    m_lastJump = jumpHeld;

    m_fpsFrames++;
    m_fpsTimer += dt;
    m_cpsTimer += dt;

    if (m_fpsTimer >= 0.5f) {
        m_fps = (int)((float)m_fpsFrames / m_fpsTimer);
        m_fpsFrames = 0;
        m_fpsTimer = 0.f;
    }
    if (m_cpsTimer >= 1.0f) {
        m_cps = (int)((float)m_cpsClicks / m_cpsTimer);
        m_cpsClicks = 0;
        m_cpsTimer = 0.f;
    }
}

void ShadowOverlay::updateValues(int progressPercent, bool spoutOpen, bool synced) {
    if (m_fps)
        m_fps->setString(fmt::format("FPS: {}", m_fps).c_str());
    if (m_cps)
        m_cps->setString(fmt::format("CPS: {}", m_cps).c_str());
    if (m_status) {
        std::string s = fmt::format("Progress: {}%  |  Spout: {}  |  Sync: {}",
            progressPercent,
            spoutOpen ? "ON" : "OFF",
            synced ? "OK" : "DRIFT");
        m_status->setString(s.c_str());
    }
}
