#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>

// ============================================================================
// LayoutGate
// ----------------------------------------------------------------------------
// Per-layer gating for the vendored XDBot layout hooks.
//
// XDBot gates everything on a SINGLE GLOBAL bool. For two-layer architecture we
// must apply layout to PRIMARY only, NEVER to SHADOW. The clean way: layout
// applies whenever the object being added belongs to a layer that is NOT the
// shadow. We detect the shadow by pointer identity (ShadowManager::shadow()).
//
// This gate is checked at THREE call sites, all of which fire on BOTH init and
// reset (so layout stays applied after death/restart — no "decor flickers back"):
//   - LevelTools::verifyLevelIntegrity
//   - PlayLayer::addObject
//   - (init-time string swap is handled separately in main.cpp::init)
// ============================================================================
class LayoutGate {
public:
    // True => the layout transform should apply to `layer`.
    // NULL layer (e.g. integrity check before any layer exists) uses the
    // thread-local s_pending flag set briefly around PlayLayer::init.
    static bool shouldApply(PlayLayer* layer);

    // Legacy entry point for hooks that don't have a layer pointer handy: true
    // if the active singleton is PRIMARY with layout enabled, OR we are inside a
    // pending-init scope for PRIMARY.
    static bool shouldApply();

    // RAII scope marking the PlayLayer currently being initialized. Used during
    // init to tag whether THIS init should swap the level string.
    struct scope {
        bool m_prevActive;
        scope(bool active);
        ~scope();
    };
};
