#pragma once

/// AudioGuard: blocks audio calls from shadow PlayLayer.
/// Shadow through onEnter also manages music → desync and crash on start-pos change.
/// Solution: flag "shadow stepping" = true during shadow update(dt) and resetLevel.
/// Hook FMODAudioEngine drops calls while flag is true.
class AudioGuard {
public:
    static AudioGuard& get();

    void setShadowStepping(bool v) { m_shadowStepping = v; }
    bool isShadowStepping() const { return m_shadowStepping; }

private:
    AudioGuard() = default;
    bool m_shadowStepping = false;
};
