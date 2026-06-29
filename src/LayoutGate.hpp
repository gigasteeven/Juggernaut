#pragma once
#include <Geode/Geode.hpp>
class PlayLayer;

namespace LayoutGate {
    // Применять layout к этому слою? PRIMARY → да, SHADOW → нет.
    bool shouldApply(PlayLayer* layer);
    // Вращает Global::get().layoutMode (который читает XDBot-хук addObject)
    void enterForPrimary();   // layoutMode = ShadowState::layoutEnabled
    void leaveForShadow();    // layoutMode = false
}