#pragma once

// ============================================================================
// Spout2 sender wrapper (pImpl).
// ----------------------------------------------------------------------------
// On Windows we link Spout2 STATICALLY from vendored sources (Spout2/). There
// is no runtime DLL to install — the sender is built into the mod .geode.
//
// IMPORTANT: this header deliberately does NOT include <Geode/Geode.hpp>. The
// vendored SpoutGL headers (SpoutSender.h -> SpoutGLextensions.h) redeclare GL
// extension function pointers with their own typedefs, which clash with the ones
// Geode's cocos2d GL headers define. Keeping Geode out of this header means TUs
// that include both this wrapper and Geode (ShadowManager.cpp, main.cpp) never
// pull SpoutSender.h transitively -> no clash. Spout.cpp is the ONLY place that
// includes SpoutSender.h, and it does not include Geode.
// ============================================================================

#include <string>

#ifdef GEODE_IS_WINDOWS
#define LAYOUT_SHADOW_SPOUT 1
#else
#define LAYOUT_SHADOW_SPOUT 0
#endif

class SpoutOutput {
public:
    SpoutOutput();
    ~SpoutOutput();

    SpoutOutput(const SpoutOutput&) = delete;
    SpoutOutput& operator=(const SpoutOutput&) = delete;

    // Create a named sender at `width`x`height`. Returns true on success.
    bool open(const std::string& name, int width, int height);

    // Push an RGBA8 frame (width*height*4 bytes). glReadPixels of an FBO yields
    // bottom-up data; Spout's SendImage with bInvert=true flips it for OBS.
    void sendFrame(const unsigned char* data, int width, int height);

    void close();
    bool isOpen() const;

private:
    // Opaque handle to the vendored ::SpoutSender (defined in Spout.cpp).
    void* m_impl = nullptr;
};
