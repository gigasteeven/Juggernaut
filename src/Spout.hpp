#pragma once

// ============================================================================
// Spout2 sender wrapper.
// ----------------------------------------------------------------------------
// On Windows we link Spout2 STATICALLY from vendored sources (Spout2/). There
// is no runtime DLL to install — the sender is built into the mod .geode. On
// non-Windows builds this is a no-op stub (CI compiles but there is no OBS
// target there anyway).
// ============================================================================

#include <Geode/Geode.hpp>
#include <string>

#ifdef GEODE_IS_WINDOWS
#define LAYOUT_SHADOW_SPOUT 1
#else
#define LAYOUT_SHADOW_SPOUT 0
#endif

#if LAYOUT_SHADOW_SPOUT
// Pull in the vendored SpoutGL sender so we can hold one by value. The header
// pulls Windows GL headers; it is only included on Windows where that is fine.
#include "SpoutSender.h"
#endif

class SpoutOutput {
public:
    SpoutOutput() = default;
    ~SpoutOutput();

    // Create a named sender at `width`x`height`. Returns true on success.
    bool open(const std::string& name, int width, int height);

    // Push an RGBA8 frame (width*height*4 bytes). glReadPixels of an FBO yields
    // bottom-up data; Spout's SendImage with bInvert=true flips it for OBS.
    void sendFrame(const unsigned char* data, int width, int height);

    void close();
    bool isOpen() const { return m_open; }

private:
#if LAYOUT_SHADOW_SPOUT
    bool m_open = false;
    int m_width = 0;
    int m_height = 0;
    ::SpoutSender m_sender;
#else
    bool m_open = false;
#endif
};
