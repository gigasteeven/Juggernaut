// LayoutGate.cpp
#include "LayoutGate.hpp"
#include "ShadowState.hpp"
#include "../vendor/xdbot_includes.hpp"
#include <Geode/Bindings.hpp>

namespace LayoutGate {
    bool shouldApply(PlayLayer* layer) {
        if (!ShadowState::get().layoutEnabled) return false;
        if (ShadowState::isShadow(layer)) return false;   // нюанс №5
        return true;
    }
    void enterForPrimary() { Global::get().layoutMode = ShadowState::get().layoutEnabled; }
    void leaveForShadow()  { Global::get().layoutMode = false; }
}