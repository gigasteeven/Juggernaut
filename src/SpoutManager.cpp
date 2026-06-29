// SpoutManager.cpp
#include "SpoutManager.hpp"
#include "ShadowState.hpp"
#include <Geode/Bindings.hpp>
#include <Geode/utils/cocos-stuff.hpp>
// Spout.h инклудится ТОЛЬКО здесь — этот TU НЕ должен тянуть Geode PCH с glew.h.
// Если CMake всё же подключит PCH — вынести send-часть в отдельный TU внутри spout2_static.
#include <Spout.h>

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

    // Рисуем shadow в offscreen render texture
    m_rt->begin();
    shadow->visit();
    // TODO: индикаторы (FPS/CPS/прогресс) — рисовать CCLabelBMFont поверх сюда.
    m_rt->end();

    // Вычитаем пиксели из текстуры
    // TODO(verify-cocos): способ вычитать пиксели из CCRenderTexture в cocos2d-x GD 2.2081.
    //   Вариант A: glReadPixels после m_rt->end() с viewport = (0,0,w,h)
    //   Вариант B: CCImage::initWithTexture(m_rt->getSprite()->getTexture())
    // На скелете — заглушка, заполнить после зелёного билда.
    std::fill(m_pixels.begin(), m_pixels.end(), 0);

    // Отправляем в Spout2 → OBS (источник Spout2 Capture)
    // bInvert=true т.к. FBO перевёрнут по Y
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