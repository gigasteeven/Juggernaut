#include "Spout.hpp"
#include <Geode/Geode.hpp>

#if LAYOUT_SHADOW_SPOUT

// SpoutSender.h is already pulled in via Spout.hpp when LAYOUT_SHADOW_SPOUT.

using namespace geode::prelude;

// ============================================================================
// Spout2 sender, linked statically from vendored SpoutGL sources.
//
// We hold a SpoutSender (from SPOUTSDK/SpoutGL/SpoutSender.h) by value. The
// public interface mirrors XDBot's texture-preview fallback: if open() fails
// for any reason, sendFrame() is a no-op and the shadow still renders to its
// offscreen texture (useful for debugging).
// ============================================================================

SpoutOutput::~SpoutOutput() {
    close();
}

bool SpoutOutput::open(const std::string& name, int width, int height) {
    close();
    if (name.empty() || width <= 0 || height <= 0) return false;

    m_sender.SetSenderName(name.c_str());

    // CreateSender registers the named sender at the given size. Subsequent
    // SendImage calls publish frames to it.
    if (!m_sender.CreateSender(name.c_str(), (unsigned int)width, (unsigned int)height)) {
        log::warn("[Shadow/Spout] CreateSender failed for '{}'", name);
        m_sender.ReleaseSender();
        return false;
    }

    m_width = width;
    m_height = height;
    m_open = true;
    log::info("[Shadow/Spout] sender '{}' opened @ {}x{}", name, width, height);
    return true;
}

void SpoutOutput::sendFrame(const unsigned char* data, int width, int height) {
    if (!m_open || !data) return;
    if (width != m_width || height != m_height) return; // caller reopens on resize
    // SendImage(pixels, w, h, GL_RGBA, bInvert). glReadPixels of an FBO yields
    // bottom-up RGBA; bInvert=true flips it so OBS sees it upright.
    m_sender.SendImage(data, (unsigned int)width, (unsigned int)height, GL_RGBA, true);
}

void SpoutOutput::close() {
    if (!m_open) return;
    m_sender.ReleaseSender();
    m_open = false;
    m_width = 0;
    m_height = 0;
}

#else // not Windows

SpoutOutput::~SpoutOutput() = default;
bool SpoutOutput::open(const std::string&, int, int) { return false; }
void SpoutOutput::sendFrame(const unsigned char*, int, int) {}
void SpoutOutput::close() {}

#endif
