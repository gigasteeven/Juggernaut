#pragma once

// ============================================================================
// Spout2 sender wrapper (Windows-only, runtime-loaded via LoadLibrary).
// ----------------------------------------------------------------------------
// Uses the official Spout2 C API (SpoutLibrary): getSpoutLibrary() returns a
// pointer to a "SPOUTLIBRARY" object exposing CreateSender / SendImage /
// ReleaseSender. Spout.dll ships with OBS / the Spout installer and is NOT
// bundled here. We dlopen it at runtime so the mod loads even when Spout is
// absent (CI, dev machines): it then degrades to "no output, texture preview".
// ============================================================================

#include <Geode/Geode.hpp>
#include <string>

#ifdef GEODE_IS_WINDOWS
#define LAYOUT_SHADOW_SPOUT 1
#else
#define LAYOUT_SHADOW_SPOUT 0
#endif

// Minimal subset of the SpoutLibrary C interface we bind to at runtime.
// Mirrors Spout2 SpoutLibrary.h "SPOUTLIBRARY".
struct SpoutLibrary;
typedef SpoutLibrary* (__cdecl* PFN_getSpoutLibrary)();

class SpoutSender {
public:
    SpoutSender() = default;
    ~SpoutSender();

    // Open a sender with `name` at `width`x`height`. No-op if Spout unavailable.
    // Returns true if actually sending.
    bool open(const std::string& name, int width, int height);

    // Push an RGBA8 frame. `data` must be width*height*4 bytes. The Spout C API
    // SendImage expects bInvert=1 for bottom-up OpenGL data (which is what we
    // get from glReadPixels of an FBO), so we pass that.
    void sendFrame(const unsigned char* data, int width, int height);

    void close();
    bool isOpen() const { return m_open; }

private:
#if LAYOUT_SHADOW_SPOUT
    bool m_open = false;
    void* m_lib = nullptr;          // HMODULE
    SpoutLibrary* m_s = nullptr;    // SPOUTLIBRARY*
    int m_width = 0;
    int m_height = 0;
#else
    bool m_open = false;
#endif
};
