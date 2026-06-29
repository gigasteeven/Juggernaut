#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../ShadowState.hpp"
#include "../ShadowManager.hpp"
#include "../SpoutManager.hpp"
#include "../LayoutGate.hpp"
// XDBot $modify(PlayLayer) из vendor/layout_mode.cpp объединяется с этим —
// Geode поддерживает несколько $modify одного класса.
using namespace geode::prelude;

class $modify(ShadowPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool b1, bool b2) {
        if (!PlayLayer::init(level, b1, b2)) return false;
        // XDBot-хук init (layout подмена m_levelString) сработал в базовом вызове.

        if (ShadowState::isShadow(this)) return true;   // shadow — ничего не делаем

        // PRIMARY
        auto& st = ShadowState::get();
        st.primary = this;
        st.layoutEnabled = Mod::get()->getSettingValue<bool>("enable-layout");
        st.spoutName     = Mod::get()->getSettingValue<std::string>("spout-name");
        st.renderWidth   = Mod::get()->getSettingValue<int64_t>("render-width");
        st.renderHeight  = Mod::get()->getSettingValue<int64_t>("render-height");
        st.logDrift      = Mod::get()->getSettingValue<bool>("log-drift");

        LayoutGate::enterForPrimary();
        ShadowManager::get().createShadow(level);
        SpoutManager::get().ensure(st.renderWidth, st.renderHeight);
        return true;
    }

    void addObject(GameObject* obj) {
        // Нюанс №5: gate проверяется КАЖДЫЙ вызов, не только в init.
        // XDBot-хук addObject читает Global::get().layoutMode — крутим его тут.
        if (ShadowState::isShadow(this)) LayoutGate::leaveForShadow();
        else                              LayoutGate::enterForPrimary();
        PlayLayer::addObject(obj);
    }

    void postUpdate(float dt) {
        // Сначала степ PRIMARY
        PlayLayer::postUpdate(dt);
        if (ShadowState::isShadow(this)) return;   // защита (shadow не должен тут быть)

        // Ручной степ shadow тем же dt и тем же числом фиксированных шагов.
        // Сначала степаем оба, ПОТОМ рендер.
        ShadowManager::get().stepShadow(dt);

        if (ShadowState::get().shadow)
            SpoutManager::get().renderShadow(ShadowState::get().shadow);
    }

    void resetLevel() {
        if (ShadowState::isShadow(this)) {
            PlayLayer::resetLevel();
            return;
        }
        // PRIMARY reset → синхронно ресетим shadow (обрамлено shadowStepping)
        PlayLayer::resetLevel();
        ShadowManager::get().resetShadow();
    }

    void onExit() {
        if (!ShadowState::isShadow(this)) {
            ShadowManager::get().destroyShadow();
            SpoutManager::get().shutdown();
            ShadowState::get().primary = nullptr;
        }
        PlayLayer::onExit();
    }
};