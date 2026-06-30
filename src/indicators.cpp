// Indicators: FPS, CPS, Progress overlay for Spout2 output.

#include "indicators.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

Indicators& Indicators::get() {
    static Indicators instance;
    return instance;
}

void Indicators::update(float dt) {
    // ── FPS ──
    m_fpsAccum += dt;
    m_fpsFrames++;
    if (m_fpsAccum >= 1.0f) {
        m_fps = m_fpsFrames;
        m_fpsFrames = 0;
        m_fpsAccum -= 1.0f;
    }

    // ── CPS: remove clicks older than 1 second ──
    float now = 0.f;
    for (auto& t : m_clickTimes) t += dt;
    m_clickTimes.erase(
        std::remove_if(m_clickTimes.begin(), m_clickTimes.end(),
            [](float t) { return t > 1.0f; }),
        m_clickTimes.end()
    );
    m_cps = static_cast<int>(m_clickTimes.size());

    // ── Progress ──
    auto pl = GameManager::sharedState()->m_playLayer;
    if (pl) {
        m_progress = pl->getCurrentPercent();
    }
}

void Indicators::registerClick() {
    m_clickTimes.push_back(0.f);
}

void Indicators::drawOnto(cocos2d::CCRenderTexture* renderTex) {
    if (!renderTex) return;

    auto mod = Mod::get();
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    float margin = 10.f;
    float yPos = winSize.height - 20.f;

    renderTex->begin();

    // FPS
    if (mod->getSettingValue<bool>("show-fps")) {
        auto fpsLabel = CCLabelBMFont::create(
            fmt::format("FPS: {}", m_fps).c_str(), "bigFont.fnt"
        );
        fpsLabel->setScale(0.4f);
        fpsLabel->setAnchorPoint({0.f, 1.f});
        fpsLabel->setPosition({margin, yPos});
        fpsLabel->setOpacity(200);
        fpsLabel->visit();
        yPos -= 25.f;
    }

    // CPS
    if (mod->getSettingValue<bool>("show-cps")) {
        auto cpsLabel = CCLabelBMFont::create(
            fmt::format("CPS: {}", m_cps).c_str(), "bigFont.fnt"
        );
        cpsLabel->setScale(0.4f);
        cpsLabel->setAnchorPoint({0.f, 1.f});
        cpsLabel->setPosition({margin, yPos});
        cpsLabel->setOpacity(200);
        cpsLabel->visit();
        yPos -= 25.f;
    }

    // Progress
    if (mod->getSettingValue<bool>("show-progress")) {
        auto progLabel = CCLabelBMFont::create(
            fmt::format("{:.1f}%", m_progress).c_str(), "bigFont.fnt"
        );
        progLabel->setScale(0.4f);
        progLabel->setAnchorPoint({0.f, 1.f});
        progLabel->setPosition({margin, yPos});
        progLabel->setOpacity(200);
        progLabel->visit();
    }

    renderTex->end();
}
