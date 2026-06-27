#pragma once

#include <Geode/Geode.hpp>

// ============================================================================
// LayoutGate
// ----------------------------------------------------------------------------
// Per-layer gating for the vendored XDBot layout hooks (Step 4).
//
// XDBot gates everything on a SINGLE GLOBAL bool (Global::get().layoutMode).
// That breaks for us: when SHADOW inits on the ORIGINAL level string, the same
// global flag would route the shadow through the layout path too. We must apply
// layout to PRIMARY only, never to SHADOW.
//
// Approach: a thread-local "layout should apply to the next PlayLayer::init" flag.
//   - PRIMARY init runs inside a LayoutGate::scope(active=true)
//   - SHADOW  init runs inside a LayoutGate::scope(active=false)
//
// The vendored LevelTools/PlayLayer/addObject hooks below read
// LayoutGate::shouldApply() instead of a global bool.
// ============================================================================
class LayoutGate {
    // True => apply layout to the PlayLayer currently being initialized.
    static thread_local bool s_active;

public:
    static bool shouldApply() { return s_active; }

    // RAII scope: set the flag for the lifetime of the guard (i.e. one init()).
    struct scope {
        bool m_prev;
        scope(bool active) {
            m_prev = s_active;
            s_active = active;
        }
        ~scope() { s_active = m_prev; }
    };
};
