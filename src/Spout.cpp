#include "Spout.hpp"
#include <Geode/Geode.hpp>

#if LAYOUT_SHADOW_SPOUT

#include <windows.h>

using namespace geode::prelude;

// ============================================================================
// Spout2 C API (SpoutLibrary) binding.
//
// getSpoutLibrary() returns a pointer to a C++ object ("SPOUTLIBRARY") whose
// methods are a virtual table. Rather than ship the full SpoutLibrary.h, we
// declare a matching abstract struct and call through the vtable. The order of
// the methods here must match Spout2 SpoutLibrary.h.
//
// We only use: CreateSender, SendImage, ReleaseSender.
//
// ⚠️ TODO (see HANDOFF BLOCK 5): verify the EXACT vtable order against the
// Spout2 SDK SpoutLibrary.h (https://github.com/leadedge/Spout2). If calls
// misbehave or crash, the safest fix is to vendor SpoutLibrary.h directly.
// ============================================================================

// Minimal GL typedefs for the vtable signatures below. We do NOT call GL here.
typedef unsigned int GLuint;

// Subset of the SPOUTLIBRARY vtable we care about. The full object has more
// methods; we only declare up to the ones we call so the vtable layout is
// correct for OUR calls. Spout2's SPOUTLIBRARY begins with these methods.
struct SpoutLibrary {
    virtual int         GetSpoutVersion() = 0;
    virtual int         OpenSpout() = 0;
    virtual bool        CreateSender(const char* sendername, unsigned int width,
                                     unsigned int height, int sharing = 3,
                                     int format = 0) = 0;
    virtual void        ReleaseSender(DWORD dwFps = 0) = 0;
    virtual bool        SendTexture(GLuint texID, GLuint fbo, unsigned int width,
                                    unsigned int height, bool bInvert = false,
                                    GLuint hostFBO = 0, const char* sendername = nullptr) = 0;
    virtual bool        SendImage(const unsigned char* pixels, unsigned int width,
                                  unsigned int height, bool bInvert = true,
                                  int hostFBO = 0, const char* sendername = nullptr) = 0;
    // ... more methods exist below; not needed.
};

// Export resolver: getSpoutLibrary is the documented entry point.
typedef SpoutLibrary* (__cdecl* PFN_getSpoutLibrary)();

SpoutSender::~SpoutSender() {
    close();
}

bool SpoutSender::open(const std::string& name, int width, int height) {
    close();
    if (name.empty() || width <= 0 || height <= 0) return false;

    // 64-bit GD -> Spout64.dll preferred, then Spout.dll.
    HMODULE lib = LoadLibraryA("Spout64.dll");
    if (!lib) lib = LoadLibraryA("Spout.dll");
    if (!lib) {
        log::warn("[Shadow/Spout] Spout DLL not found; falling back to texture preview. "
                  "Install Spout2 (ships with OBS) to enable OBS output.");
        return false;
    }

    auto getLib = reinterpret_cast<PFN_getSpoutLibrary>(
        GetProcAddress(lib, "getSpoutLibrary"));
    if (!getLib) {
        log::warn("[Shadow/Spout] Spout DLL present but missing 'getSpoutLibrary' export.");
        FreeLibrary(lib);
        return false;
    }

    SpoutLibrary* s = getLib();
    if (!s) {
        log::warn("[Shadow/Spout] getSpoutLibrary() returned null.");
        FreeLibrary(lib);
        return false;
    }

    // CreateSender(name, w, h, sharing=3 texture+share, format=0 default BGRA).
    // Spout accepts CPU-side RGBA via SendImage regardless of this format.
    if (!s->CreateSender(name.c_str(), (unsigned int)width, (unsigned int)height)) {
        log::warn("[Shadow/Spout] CreateSender failed for '{}'", name);
        FreeLibrary(lib);
        return false;
    }

    m_lib = lib;
    m_s = s;
    m_width = width;
    m_height = height;
    m_open = true;
    log::info("[Shadow/Spout] sender '{}' opened @ {}x{}", name, width, height);
    return true;
}

void SpoutSender::sendFrame(const unsigned char* data, int width, int height) {
    if (!m_open || !m_s || !data) return;
    if (width != m_width || height != m_height) return; // caller reopens on resize
    // bInvert=true: glReadPixels of an FBO is bottom-up, Spout expects that.
    m_s->SendImage(data, (unsigned int)width, (unsigned int)height, true);
}

void SpoutSender::close() {
    if (!m_open) return;
    if (m_s) m_s->ReleaseSender();
    if (m_lib) FreeLibrary(static_cast<HMODULE>(m_lib));
    m_open = false;
    m_lib = nullptr;
    m_s = nullptr;
    m_width = 0;
    m_height = 0;
}

#else // not Windows

SpoutSender::~SpoutSender() = default;
bool SpoutSender::open(const std::string&, int, int) { return false; }
void SpoutSender::sendFrame(const unsigned char*, int, int) {}
void SpoutSender::close() {}

#endif
