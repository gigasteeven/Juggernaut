#pragma once
#include <Geode/Geode.hpp>
using namespace geode::prelude;

class PlayLayer;

class ShadowState {
    ShadowState() = default;
public:
    static ShadowState& get();

    // Идентификация shadow по указателю (нюанс №5 — gate в addObject)
    static bool isShadow(PlayLayer* layer);

    PlayLayer* primary = nullptr;
    PlayLayer* shadow  = nullptr;   // offscreen, НЕ singleton, удерживается retain()

    // Флаги синхронизации (критично для аудио)
    bool shadowStepping  = false;   // true только вокруг update(dt)/resetLevel для shadow
    bool shadowResetting = false;

    // Настройки
    bool        layoutEnabled = true;
    bool        logDrift      = true;
    std::string spoutName     = "GDShadow";
    int         renderWidth   = 1920;
    int         renderHeight  = 1080;
};