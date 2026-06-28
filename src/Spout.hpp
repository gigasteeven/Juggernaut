#pragma once

// ============================================================================
// Spout2 sender wrapper (pImpl).
// ----------------------------------------------------------------------------
// On Windows we link Spout2 STATICALLY from vendored sources (Spout2/). There
// is no runtime DLL to install — the sender is built into the mod .geode.
//
// IMPORTANT: this header deliberately does NOT include <Geode/Geode.hpp> and
// does NOT include SpoutSender.h. The vendored SpoutGL headers redeclare GL
// extension function pointers with their own typedefs, which clash with Geode's
// cocos2d GL headers (and Geode's force-included precompiled header). The real
// ::SpoutSender is therefore held behind an opaque forward-declared Impl and
// lives only in Spout.cpp, which is compiled as part of a SEPARATE static
// library that does not use Geode's PCH.
// ============================================================================

#include <string>

// LAYOUT_SHADOW_SPOUT is also injected by CMake for the Spout build targets,
// but define it here as a fallback so the public header is self-contained.
#ifndef LAYOUT_SHADOW_SPOUT
#ifdef _WIN32
#define LAYOUT_SHADOW_SPOUT 1
#else
#define LAYOUT_SHADOW_SPOUT 0
#endif
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
    struct Impl;                 // defined in Spout.cpp
    Impl* m_impl = nullptr;      // not used on non-Windows
};
