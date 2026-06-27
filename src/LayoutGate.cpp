#include "LayoutGate.hpp"

// The shadow PlayLayer pointer, registered by ShadowManager on create/clear.
// Layout-only logic: never dereferenced, only compared by identity. Keeping it
// here (instead of #include "ShadowManager.hpp") avoids a header cycle.
static PlayLayer* g_shadowLayer = nullptr;
static thread_local bool s_pendingActive = false;

// C-linkage accessor so ShadowManager can register/unregister the shadow layer
// without dragging in a header cycle.
extern "C" void LayoutGate_markShadow(PlayLayer* p) { g_shadowLayer = p; }

bool LayoutGate::shouldApply(PlayLayer* layer) {
    if (!layer) return s_pendingActive;
    // Layout applies to everything EXCEPT the shadow layer.
    return layer != g_shadowLayer;
}

bool LayoutGate::shouldApply() {
    // For hooks that have no layer pointer: if a shadow exists, PRIMARY needs
    // layout; otherwise fall back to the pending-init scope flag.
    if (g_shadowLayer != nullptr) return true;
    return s_pendingActive;
}

LayoutGate::scope::scope(bool active) : m_prevActive(s_pendingActive) {
    s_pendingActive = active;
}
LayoutGate::scope::~scope() {
    s_pendingActive = m_prevActive;
}
