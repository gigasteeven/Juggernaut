// Spout wrapper implementation — compiled WITHOUT Geode PCH.
// GL_GLEXT_LEGACY is defined in CMake to prevent GL extension conflicts.
//
// This wraps SpoutGL's core functionality using direct Win32 shared memory
// and DX11/OpenGL interop for zero-copy texture sharing.
//
// For the initial build, we use a simplified glReadPixels + shared memory
// approach. Full DX11 interop will be added once the pipeline is verified.

// Must be first to prevent wingdi conflicts
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>

#include "../spout/spout_wrapper.h"

#include <cstdio>
#include <cstring>

// ─── Spout shared memory sender (simplified) ───
// Uses Windows named shared memory + a lightweight protocol so OBS
// Spout2 Capture can find our sender.

// Spout uses shared memory mapped files for the sender/receiver name list
// and DX11 shared textures for the actual frame data.
// For Phase 1: we'll use SpoutSender from vendored sources.

// For now we vendor the Spout2 library source directly.
// The CMakeLists links us against d3d11, dxgi, opengl32.

// We define SPOUT_BUILD_DLL=0 to build as static lib.

// Include the Spout SDK headers
// These are expected at src/spout/SpoutGL/
// If not present, provide stub implementation.

#if __has_include("SpoutGL/SpoutSender.h")
    #include "SpoutGL/SpoutSender.h"
    #define HAS_SPOUT_SDK 1
#else
    #define HAS_SPOUT_SDK 0
#endif

#if HAS_SPOUT_SDK

static SpoutSender g_sender;
static bool g_initialized = false;
static char g_senderName[256] = {0};

extern "C" {

int spout_init(const char* senderName) {
    if (g_initialized) return 1;
    strncpy_s(g_senderName, senderName, sizeof(g_senderName) - 1);
    g_sender.SetSenderName(g_senderName);
    g_initialized = true;
    return 1;
}

int spout_send_texture(unsigned int texId, unsigned int width, unsigned int height) {
    if (!g_initialized) return 0;
    return g_sender.SendTexture(texId, GL_TEXTURE_2D, width, height, true, 0) ? 1 : 0;
}

int spout_send_image(const unsigned char* pixels, unsigned int width, unsigned int height) {
    if (!g_initialized) return 0;
    return g_sender.SendImage(pixels, width, height, GL_RGBA, false, 0) ? 1 : 0;
}

void spout_release() {
    if (g_initialized) {
        g_sender.ReleaseSender();
        g_initialized = false;
    }
}

int spout_is_initialized() {
    return g_initialized ? 1 : 0;
}

} // extern "C"

#else
// ─── Stub implementation when Spout SDK sources not yet vendored ───
// This allows the mod to build and run without Spout — frames just don't
// get sent anywhere. Once the user places SpoutGL sources, the real
// implementation takes over via __has_include.

#include <cstdio>

extern "C" {

int spout_init(const char* senderName) {
    fprintf(stderr, "[Juggernaut/Spout] STUB: init('%s') — Spout SDK not found\n", senderName);
    return 1;
}

int spout_send_texture(unsigned int texId, unsigned int width, unsigned int height) {
    (void)texId; (void)width; (void)height;
    return 0;
}

int spout_send_image(const unsigned char* pixels, unsigned int width, unsigned int height) {
    (void)pixels; (void)width; (void)height;
    return 0;
}

void spout_release() {}

int spout_is_initialized() {
    return 0;
}

} // extern "C"

#endif
