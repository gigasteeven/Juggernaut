#include <Geode/Geode.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include "../ShadowState.hpp"
using namespace geode::prelude;

class $modify(ShadowFMOD, FMODAudioEngine) {
    // TODO(verify-bro): точные сигнатуры для 2.2081 — подтвердить на первом билде по логу CI.
    void playEffect(gd::string path, float speed, float pitch, float volume) {
        if (ShadowState::get().shadowStepping) return;   // дроп — звук только из PRIMARY
        FMODAudioEngine::playEffect(path, speed, pitch, volume);
    }

    void setMusicTimeMS(int time) {
        if (ShadowState::get().shadowStepping || ShadowState::get().shadowResetting) return;
        FMODAudioEngine::setMusicTimeMS(time);
    }
};