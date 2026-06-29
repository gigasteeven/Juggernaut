// SpoutManager.hpp
#pragma once
#include <Geode/Geode.hpp>
class PlayLayer;

class SpoutManager {
public:
    static SpoutManager& get();
    void ensure(unsigned w, unsigned h);
    void renderShadow(PlayLayer* shadow);   // шагнули → рендерим → SendImage
    void shutdown();
private:
    SpoutManager() = default;
    void* m_spout = nullptr;          // Spout* — скрываем, чтобы не тащить Spout.h в Geode TU
    cocos2d::CCRenderTexture* m_rt = nullptr;
    std::vector<unsigned char> m_pixels;
    unsigned m_w = 0, m_h = 0;
    bool m_ready = false;
};