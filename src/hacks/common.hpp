#pragma once
// Include comuni per i file delle funzioni
#include "config.hpp"
#include "events.hpp"
#include "lang.hpp"
#include "registry.hpp"
#include "core/hooks.hpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "gd/cocos.hpp"
#include "gd/gd.hpp"
#include "gui/gui.hpp"
#include "gui/menu.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace mh {

// Itera un CCArray di cocos
template <class F>
inline void forEach(void* arr, F f) {
    unsigned n = cc::arrayCount(arr);
    for (unsigned i = 0; i < n; i++) f(cc::arrayAt(arr, i));
}

inline gd::GameObjectType objType(void* obj) { return GDF(obj, GameObject, m_objectType, gd::GameObjectType); }

inline bool isHazard(gd::GameObjectType t) {
    return t == gd::GameObjectType::Hazard || t == gd::GameObjectType::AnimatedHazard;
}
inline bool isSolid(gd::GameObjectType t) {
    return t == gd::GameObjectType::Solid || t == gd::GameObjectType::Breakable;
}
inline bool isSlope(gd::GameObjectType t) { return t == gd::GameObjectType::Slope; }
inline bool isSpecial(gd::GameObjectType t) {
    return !isHazard(t) && !isSolid(t) && !isSlope(t) && t != gd::GameObjectType::Decoration &&
           t != gd::GameObjectType::Modifier;
}

// Stato del gameState dentro GJBaseGameLayer
template <class T>
inline T& gameState(void* layer, uintptr_t member) {
    return mem::at<T>(layer, gd::off::GJBaseGameLayer::m_gameState + member);
}

// Uccide il giocatore (bypassa noclip)
void killPlayer(void* pl, void* player);
extern bool g_forceDeath;

// Dialog di conferma (ImGui) - usato dalle opzioni "Confirm ..."
void confirm(const std::string& message, void (*onYes)(void*), void* ctx);

} // namespace mh
