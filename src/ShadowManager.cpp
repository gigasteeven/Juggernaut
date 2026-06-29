// src/ShadowManager.cpp
#include "ShadowManager.hpp"
#include "ShadowState.hpp"
#include "LayoutGate.hpp"
#include <Geode/Bindings.hpp>
#include <Geode/utils/cocos-stuff.hpp>

ShadowManager& ShadowManager::get() { static ShadowManager m; return m; }

void ShadowManager::createShadow(GJGameLevel* level) {
    auto& st = ShadowState::get();
    if (st.shadow) destroyShadow();

    LayoutGate::leaveForShadow();

    auto* gm = GameManager::get();
    PlayLayer* savedPrimary = gm->m_playLayer;

    // PlayLayer::create для 2.2081
    PlayLayer* sh = PlayLayer::create(level, false, false);
    if (!sh) { LayoutGate::enterForPrimary(); return; }

    sh->retain();
    gm->m_playLayer = savedPrimary; // Восстанавливаем PRIMARY (нюанс №6)

    st.shadow = sh;

    sh->onEnter();
    sh->unscheduleUpdate(); // Отключаем авто-тик (иначе x2 скорость)

    LayoutGate::enterForPrimary();
    log::info("SHADOW created, retained, unscheduleUpdate done");
}

void ShadowManager::destroyShadow() {
    auto& st = ShadowState::get();
    if (!st.shadow) return;
    st.shadow->onExit();
    st.shadow->release();
    st.shadow = nullptr;
}

void ShadowManager::stepShadow(float dt) {
    auto& st = ShadowState::get();
    if (!st.shadow) return;

    // Обрамляем update shadow флагом (критично для аудио)
    st.shadowStepping = true;
    st.shadow->update(dt);
    st.shadowStepping = false;

    // Debug: Сравнение позиций PRIMARY и SHADOW
    if (st.logDrift && st.primary && st.shadow->m_player1 && st.primary->m_player1) {
        auto p1 = st.primary->m_player1->getPosition();
        auto p2 = st.shadow->m_player1->getPosition();
        
        float drift = ccpDistance(p1, p2);
        
        // Drift ~0.7 это норма (sub-frame rounding GD). Логируем только если больше 5.0
        if (drift > 5.0f) {
            log::warn("Drift detected: {:.2f} (P1: {:.1f},{:.1f} | SHADOW: {:.1f},{:.1f})", 
                drift, p1.x, p1.y, p2.x, p2.y);
        }
    }
}

void ShadowManager::resetShadow() {
    auto& st = ShadowState::get();
    if (!st.shadow) return;
    st.shadowResetting = true;
    st.shadowStepping  = true;
    st.shadow->resetLevel();
    st.shadowStepping  = false;
    st.shadowResetting = false;
}