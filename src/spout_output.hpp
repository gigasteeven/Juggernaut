#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

/// SpoutOutput: manages the Spout2 sender lifecycle and frame sending.
/// In PlayLayer: captures shadow render texture → sends via Spout.
/// In menus/editor: captures the screen directly → sends via Spout (1:1 mirror).
class SpoutOutput {
public:
    static SpoutOutput& get();

    /// Initialize Spout sender.
    void init();

    /// Send a frame from the shadow render texture.
    void sendShadowFrame();

    /// Send current screen (for menu/editor mirroring).
    void sendScreenFrame();

    /// Cleanup Spout sender.
    void shutdown();

    bool isActive() const { return m_active; }

private:
    SpoutOutput() = default;

    bool m_active = false;
    std::vector<unsigned char> m_pixelBuffer;
    int m_lastWidth = 0;
    int m_lastHeight = 0;
};
