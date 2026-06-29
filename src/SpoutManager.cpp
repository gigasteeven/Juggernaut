// src/SpoutManager.cpp
#include "SpoutManager.hpp"
#include "ShadowState.hpp"
#include <Geode/Bindings.hpp>
#include <Geode/utils/cocos-stuff.hpp>
#include <Spout.h>
#include <regex>

SpoutManager& SpoutManager::get() { static SpoutManager m; return m; }

void SpoutManager::ensure(unsigned w, unsigned h) {
    if (!m_ready) {
        auto& st = ShadowState::get();
        auto* sp = new Spout();
        sp->SetSenderName(st.spoutName.c_str());
        m_spout = sp;
        m_w = w; m_h = h;
        m_pixels.resize(w * h * 4);
        m_ready = true;
    }
    if (!m_rt || m_w != w || m_h != h) {
        CC_SAFE_RELEASE(m_rt);
        m_rt = cocos2d::CCRenderTexture::create(w, h, kCCTexture2DPixelFormat_RGBA8888);
        m_rt->retain();
        m_w = w; m_h = h;
    }
}

void SpoutManager::renderShadow(PlayLayer* shadow) {
    if (!shadow || !m_rt || !m_ready) return;

    // 1. Рисуем PlayLayer в offscreen текстуру
    m_rt->begin();
    shadow->visit();

    // 2. Индикаторы поверх (FPS, CPS, Прогресс)
    // Простая реализация: читаем CPS и FPS из глобальных полей GD
    auto gm = GameManager::get();
    float fps = gm->m_fps;
    // TODO: добавить реальный CPS, если есть глобальный счетчик, пока заглушка 0
    int cps = 0; 
    float progress = 0.0f;
    if (shadow->m_player1) {
        // Примерный расчет прогресса (нужно подтвердить поле m_levelLength в 2.2081)
        // progress = shadow->m_player1->getPositionX() / shadow->m_levelLength;
    }

    std::string text = fmt::format("FPS: {:.0f} | CPS: {} | {:.1f}%", fps, cps, progress * 100.f);
    auto label = cocos2d::CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
    label->setAnchorPoint({0, 1});
    label->setPosition({10, (float)m_h - 10});
    label->setZOrder(9999);
    label->visit();

    m_rt->end();

    // 3. Читаем пиксели из FBO
    // glReadPixels медленный, но для старта сойдет. Позже заменим на PBO.
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_w, m_h, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels.data());

    // 4. Отправляем в Spout2 (bInvert = true, т.к. FBO перевернут по Y)
    static_cast<Spout*>(m_spout)->SendImage(
        m_pixels.data(), m_w, m_h, GL_RGBA, true, 0
    );
}

void SpoutManager::shutdown() {
    if (m_spout) {
        static_cast<Spout*>(m_spout)->ReleaseSender();
        delete static_cast<Spout*>(m_spout);
        m_spout = nullptr;
    }
    CC_SAFE_RELEASE_NULL(m_rt);
    m_pixels.clear();
    m_ready = false;
}