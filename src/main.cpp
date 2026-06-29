#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include "layout_mode.hpp"

// SpoutSender.h is included AFTER defining GL_GLEXT_LEGACY to prevent
// glew.h redeclaration conflicts with Geode PCH.
// This file is compiled in the Spout2 static lib (no Geode PCH there),
// but we still need the header here for the type.
#define GL_GLEXT_LEGACY
#include <SpoutSender.h>

using namespace geode::prelude;

// Global state for the dual PlayLayer architecture
struct DualLayerState {
    PlayLayer* primary = nullptr;
    PlayLayer* shadow = nullptr;
    bool shadowStepping = false;
    SpoutSender* spoutSender = nullptr;
    cocos2d::CCRenderTexture* renderTexture = nullptr;
    
    static DualLayerState& get() {
        static DualLayerState instance;
        return instance;
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        bool isShadow = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        auto& state = DualLayerState::get();
        
        // If primary is already set and we're initializing another PlayLayer, it must be the shadow
        if (state.primary != nullptr && state.shadow == nullptr) {
            m_fields->isShadow = true;
            state.shadow = static_cast<PlayLayer*>(static_cast<CCLayer*>(this));
            
            // Retain shadow so it doesn't get destroyed
            this->retain();
            
            // Let the shadow initialize normally (full level)
            if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
                return false;
            }
            
            // Restore m_playLayer to primary after shadow creation
            GameManager::get()->m_playLayer = state.primary;
            return true;
        }
        
        // Otherwise, this is the primary layer
        state.primary = static_cast<PlayLayer*>(static_cast<CCLayer*>(this));
        m_fields->isShadow = false;
        
        bool layoutMode = Mod::get()->getSavedValue<bool>("macro_layout_mode");
        std::string oldString = level->m_levelString;
        
        if (layoutMode) {
            level->m_levelString = LayoutMode::getModifiedString(level->m_levelString);
        }
        
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            if (layoutMode) level->m_levelString = oldString;
            return false;
        }
        
        if (layoutMode) {
            level->m_levelString = oldString;
        }
        
        // Create shadow layer
        // We temporarily set m_playLayer to nullptr so shadow init doesn't think it's primary
        GameManager::get()->m_playLayer = nullptr;
        
        // Seed synchronization happens implicitly because they are created back-to-back
        // but we should ensure they start at the same state
        auto shadow = PlayLayer::create(level, useReplay, dontCreateObjects);
        
        // Ensure m_playLayer is primary
        GameManager::get()->m_playLayer = state.primary;
        
        if (shadow) {
            // Unschedule update for shadow so it doesn't tick twice
            shadow->unscheduleUpdate();
            // Call onEnter to initialize properly
            shadow->onEnter();
            
            // Setup offscreen render texture for shadow
            auto winSize = CCDirector::get()->getWinSize();
            state.renderTexture = CCRenderTexture::create(winSize.width, winSize.height, cocos2d::kCCTexture2DPixelFormat_RGBA8888);
            state.renderTexture->retain();
            
            // Initialize Spout
            if (!state.spoutSender) {
                state.spoutSender = new SpoutSender();
                state.spoutSender->SetSenderName("GD_Shadow");
            }
        }
        
        return true;
    }
    
    void onExit() {
        auto& state = DualLayerState::get();
        if (m_fields->isShadow) {
            PlayLayer::onExit();
            return;
        }
        
        // Cleanup shadow when primary exits
        if (state.shadow) {
            state.shadow->onExit();
            state.shadow->release();
            state.shadow = nullptr;
        }
        
        if (state.renderTexture) {
            state.renderTexture->release();
            state.renderTexture = nullptr;
        }
        
        if (state.spoutSender) {
            state.spoutSender->ReleaseSender();
            delete state.spoutSender;
            state.spoutSender = nullptr;
        }
        
        state.primary = nullptr;
        PlayLayer::onExit();
    }
    
    void addObject(GameObject* obj) {
        if (m_fields->isShadow) {
            PlayLayer::addObject(obj);
            return;
        }
        
        bool layoutMode = Mod::get()->getSavedValue<bool>("macro_layout_mode");
        if (!layoutMode) {
            PlayLayer::addObject(obj);
            return;
        }
        
        if (excludedTriggerIDs.contains(obj->m_objectID)) return;
        
        PlayLayer::addObject(obj);
        
        obj->m_activeMainColorID = -1;
        obj->m_activeDetailColorID = -1;
        obj->m_detailUsesHSV = false;
        obj->m_baseUsesHSV = false;
        obj->m_hasNoGlow = true;
        obj->m_isHide = obj->m_objectID == 2065;
        obj->setOpacity(obj->m_objectID == 2065 ? 0 : 255);
        obj->setVisible(obj->m_objectID != 2065);
    }
    
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        
        auto& state = DualLayerState::get();
        if (m_fields->isShadow || !state.shadow) return;
        
        // Sync shadow with primary
        state.shadowStepping = true;
        
        // Sync player input
        state.shadow->m_player1->m_isHolding = m_player1->m_isHolding;
        state.shadow->m_player1->m_isHolding2 = m_player1->m_isHolding2;
        if (m_player2 && state.shadow->m_player2) {
            state.shadow->m_player2->m_isHolding = m_player2->m_isHolding;
            state.shadow->m_player2->m_isHolding2 = m_player2->m_isHolding2;
        }
        
        // Step shadow manually
        state.shadow->update(dt);
        
        state.shadowStepping = false;
        
        // Drift debug check
        if (std::abs(m_player1->getPositionX() - state.shadow->m_player1->getPositionX()) > 1.0f) {
            log::warn("Desync detected! Primary X: {}, Shadow X: {}", m_player1->getPositionX(), state.shadow->m_player1->getPositionX());
        }
        
        // Render shadow to texture and send via Spout
        if (state.renderTexture && state.spoutSender) {
            state.renderTexture->beginWithClear(0, 0, 0, 1);
            state.shadow->visit();
            state.renderTexture->end();
            
            // Send texture to Spout
            auto texture = state.renderTexture->getSprite()->getTexture();
            if (texture) {
                state.spoutSender->SendTexture(
                    texture->getName(),
                    GL_TEXTURE_2D,
                    texture->getPixelsWide(),
                    texture->getPixelsHigh(),
                    false, // invert
                    0      // hostFBO
                );
            }
        }
    }
    
    void resetLevel() {
        PlayLayer::resetLevel();
        
        auto& state = DualLayerState::get();
        if (!m_fields->isShadow && state.shadow) {
            state.shadowStepping = true;
            state.shadow->resetLevel();
            state.shadowStepping = false;
        }
    }
};

class $modify(FMODAudioEngine) {
    void playEffect(gd::string p0, float p1, float p2, float p3) {
        if (DualLayerState::get().shadowStepping) return;
        FMODAudioEngine::playEffect(p0, p1, p2, p3);
    }
    
    void setMusicTimeMS(unsigned int p0, bool p1, int p2) {
        if (DualLayerState::get().shadowStepping) return;
        FMODAudioEngine::setMusicTimeMS(p0, p1, p2);
    }
};
