// SpoutOutput: Connects shadow render pipeline to Spout2 sender.

#include "spout_output.hpp"
#include "shadow_layer.hpp"
#include "spout/spout_wrapper.h"
#include <Geode/Geode.hpp>

// OpenGL for glReadPixels
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

using namespace geode::prelude;

SpoutOutput& SpoutOutput::get() {
    static SpoutOutput instance;
    return instance;
}

void SpoutOutput::init() {
    if (m_active) return;

    auto senderName = Mod::get()->getSettingValue<std::string>("spout-name");
    if (spout_init(senderName.c_str())) {
        m_active = true;
        log::info("[Juggernaut] Spout sender '{}' initialized", senderName);
    } else {
        log::error("[Juggernaut] Failed to initialize Spout sender");
    }
}

void SpoutOutput::sendShadowFrame() {
    if (!m_active) return;

    auto& shadow = ShadowLayer::get();
    if (!shadow.isActive()) return;

    int w = shadow.getWidth();
    int h = shadow.getHeight();

    // Get the texture ID from the shadow render texture
    GLuint texId = shadow.getFboID();
    if (texId == 0) return;

    // Try sending texture directly (zero-copy if Spout SDK is available)
    if (spout_send_texture(texId, w, h)) return;

    // Fallback: read pixels and send as image
    if (w != m_lastWidth || h != m_lastHeight) {
        m_pixelBuffer.resize(w * h * 4);
        m_lastWidth = w;
        m_lastHeight = h;
    }

    // Bind the shadow texture and read pixels
    glBindTexture(GL_TEXTURE_2D, texId);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pixelBuffer.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    spout_send_image(m_pixelBuffer.data(), w, h);
}

void SpoutOutput::sendScreenFrame() {
    if (!m_active) return;

    auto winSize = CCDirector::sharedDirector()->getWinSizeInPixels();
    int w = static_cast<int>(winSize.width);
    int h = static_cast<int>(winSize.height);

    if (w != m_lastWidth || h != m_lastHeight) {
        m_pixelBuffer.resize(w * h * 4);
        m_lastWidth = w;
        m_lastHeight = h;
    }

    // Read the current framebuffer (screen)
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, m_pixelBuffer.data());

    spout_send_image(m_pixelBuffer.data(), w, h);
}

void SpoutOutput::shutdown() {
    if (!m_active) return;
    spout_release();
    m_active = false;
    m_pixelBuffer.clear();
    log::info("[Juggernaut] Spout sender released");
}
