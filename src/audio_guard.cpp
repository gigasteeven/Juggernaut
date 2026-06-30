// AudioGuard: hooks FMODAudioEngine to drop audio calls from shadow.

#include <Geode/Geode.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include "audio_guard.hpp"

using namespace geode::prelude;

AudioGuard& AudioGuard::get() {
    static AudioGuard instance;
    return instance;
}

class $modify(JuggFMOD, FMODAudioEngine) {
    // Hook the main playEffect that most other overloads delegate to.
    // Signature from Geode bindings: playEffect(gd::string path, float speed, float p2, float volume)
    void playEffect(gd::string path, float speed, float p2, float volume) {
        if (AudioGuard::get().isShadowStepping()) return;
        FMODAudioEngine::playEffect(path, speed, p2, volume);
    }

    // setMusicTimeMS(unsigned int ms, bool fade, int channel)
    void setMusicTimeMS(unsigned int ms, bool fade, int channel) {
        if (AudioGuard::get().isShadowStepping()) return;
        FMODAudioEngine::setMusicTimeMS(ms, fade, channel);
    }
};
