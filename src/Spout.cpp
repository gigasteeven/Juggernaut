#if LAYOUT_SHADOW_SPOUT

// ============================================================================
// Spout2 sender, linked statically from vendored SpoutGL sources.
//
// The SpoutGL headers (SpoutSender.h -> SpoutGLextensions.h) redeclare GL
// extension function pointers with their own typedefs, which clash with the ones
// Geode's cocos2d GL headers define. This TU therefore includes SpoutSender.h
// and does NOT include <Geode/Geode.hpp> at all (log goes via stderr) so the
// clash never happens anywhere. The clash is fully contained to the vendored
// SpoutGL sources, which only include their own GL headers.
// ============================================================================
#include "SpoutSender.h"

#include "Spout.hpp"
#include <cstdio>
#include <cstdarg>

namespace {
void spoutLog(const char* level, const char* fmt, ...) {
    std::fprintf(stderr, "[Shadow/Spout] %s: ", level);
    va_list ap; va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fputc('\n', stderr);
}
}

// ::SpoutSender is the vendored SpoutGL class. We keep it by value inside an
// aligned buffer so there is no extra heap allocation and no header leak.
struct SpoutOutput::Impl {
    ::SpoutSender sender;
    bool open = false;
    int width = 0;
    int height = 0;
};

SpoutOutput::SpoutOutput() {
    m_impl = new Impl();
}

SpoutOutput::~SpoutOutput() {
    close();
    delete static_cast<Impl*>(m_impl);
    m_impl = nullptr;
}

bool SpoutOutput::open(const std::string& name, int width, int height) {
    auto* impl = static_cast<Impl*>(m_impl);
    if (!impl) return false;

    // Close anything previously open.
    if (impl->open) {
        impl->sender.ReleaseSender();
        impl->open = false;
    }

    if (name.empty() || width <= 0 || height <= 0) return false;

    impl->sender.SetSenderName(name.c_str());

    // CreateSender registers the named sender at the given size. Subsequent
    // SendImage calls publish frames to it.
    if (!impl->sender.CreateSender(name.c_str(), (unsigned int)width, (unsigned int)height)) {
        spoutLog("WARN", "CreateSender failed for '%s'", name.c_str());
        impl->sender.ReleaseSender();
        return false;
    }

    impl->width = width;
    impl->height = height;
    impl->open = true;
    spoutLog("INFO", "sender '%s' opened @ %dx%d", name.c_str(), width, height);
    return true;
}

void SpoutOutput::sendFrame(const unsigned char* data, int width, int height) {
    auto* impl = static_cast<Impl*>(m_impl);
    if (!impl || !impl->open || !data) return;
    if (width != impl->width || height != impl->height) return; // caller reopens on resize
    // SendImage(pixels, w, h, GL_RGBA, bInvert). glReadPixels of an FBO yields
    // bottom-up RGBA; bInvert=true flips it so OBS sees it upright.
    impl->sender.SendImage(data, (unsigned int)width, (unsigned int)height, GL_RGBA, true);
}

void SpoutOutput::close() {
    auto* impl = static_cast<Impl*>(m_impl);
    if (!impl || !impl->open) return;
    impl->sender.ReleaseSender();
    impl->open = false;
    impl->width = 0;
    impl->height = 0;
}

bool SpoutOutput::isOpen() const {
    auto* impl = static_cast<Impl*>(m_impl);
    return impl && impl->open;
}

#else // not Windows

SpoutOutput::SpoutOutput() = default;
SpoutOutput::~SpoutOutput() = default;
bool SpoutOutput::open(const std::string&, int, int) { return false; }
void SpoutOutput::sendFrame(const unsigned char*, int, int) {}
void SpoutOutput::close() {}
bool SpoutOutput::isOpen() const { return false; }

#endif
