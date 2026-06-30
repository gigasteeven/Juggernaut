#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

/// Manages the shadow (offscreen) PlayLayer used for Spout2 output.
/// Shadow is a second PlayLayer of the same level, retain()'d, NOT the singleton.
/// It renders full level (no layout) into an offscreen CCRenderTexture.
class ShadowLayer {
public:
    static ShadowLayer& get();

    /// Create shadow from the same level as primary.
    /// Called from PlayLayer::init hook after primary is fully initialized.
    void createShadow(GJGameLevel* level, bool useReplay, bool dontCreateObjects);

    /// Destroy shadow and release resources.
    void destroyShadow();

    /// Step shadow with the same dt as primary (called from postUpdate hook).
    void stepShadow(float dt);

    /// Render shadow into offscreen texture and return the texture.
    cocos2d::CCTexture2D* renderFrame();

    /// Reset shadow (on level restart).
    void resetShadow();

    /// Is shadow alive?
    bool isActive() const { return m_shadow != nullptr; }

    /// Get the shadow PlayLayer pointer (for gate checks).
    PlayLayer* getShadow() const { return m_shadow; }

    /// Get render texture dimensions.
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    /// Get the offscreen FBO ID for Spout SendFbo.
    GLuint getFboID() const;

    /// Flag: true while we're stepping shadow (used by audio guard).
    bool isStepping() const { return m_stepping; }

private:
    ShadowLayer() = default;

    PlayLayer* m_shadow = nullptr;
    cocos2d::CCRenderTexture* m_renderTex = nullptr;
    int m_width = 1920;
    int m_height = 1080;
    bool m_stepping = false;
};
