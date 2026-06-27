#include "Overlay.hpp"

void ShadowOverlay::setup(int width, int height) {
    m_width = width;
    m_height = height;

    if (!m_fpsLabel) {
        m_fpsLabel = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        m_fpsLabel->retain();
        m_fpsLabel->setAnchorPoint({0.f, 1.f});
        m_fpsLabel->setScale(1.1f);
        m_fpsLabel->setColor({255, 255, 255});

        m_cpsLabel = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        m_cpsLabel->retain();
        m_cpsLabel->setAnchorPoint({0.f, 1.f});
        m_cpsLabel->setScale(1.1f);
        m_cpsLabel->setColor({255, 255, 255});

        m_statusLabel = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        m_statusLabel->retain();
        m_statusLabel->setAnchorPoint({0.f, 1.f});
        m_statusLabel->setScale(0.8f);
        m_statusLabel->setColor({180, 180, 180});
    }

    // Top-left stack. CCRenderTexture draws with (0,0) bottom-left.
    const float margin = 12.f;
    m_fpsLabel->setPosition({margin, (float)m_height - margin});
    m_cpsLabel->setPosition({margin, (float)m_height - margin - 32.f});
    m_statusLabel->setPosition({margin, (float)m_height - margin - 64.f});
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
    if (m_fpsLabel)
        m_fpsLabel->setString(fmt::format("FPS: {}", m_fps).c_str());
    if (m_cpsLabel)
        m_cpsLabel->setString(fmt::format("CPS: {}", m_cps).c_str());
    if (m_statusLabel) {
        std::string s = fmt::format("Progress: {}%  |  Spout: {}  |  Sync: {}",
            progressPercent,
            spoutOpen ? "ON" : "OFF",
            synced ? "OK" : "DRIFT");
        m_statusLabel->setString(s.c_str());
    }
}
