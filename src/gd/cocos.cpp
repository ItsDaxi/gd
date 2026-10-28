#include "cocos.hpp"

#include <windows.h>
#include <string>
#include <unordered_map>
#include <mutex>

namespace cc {

static HMODULE cocosModule() {
    static HMODULE m = GetModuleHandleW(L"libcocos2d.dll");
    return m;
}

void* resolve(const char* symbol) {
    static std::unordered_map<std::string, void*> cache;
    static std::mutex mtx;
    std::lock_guard lock(mtx);
    auto it = cache.find(symbol);
    if (it != cache.end()) return it->second;
    void* p = cocosModule() ? reinterpret_cast<void*>(GetProcAddress(cocosModule(), symbol)) : nullptr;
    cache.emplace(symbol, p);
    return p;
}

bool hasExport(const char* symbol) { return resolve(symbol) != nullptr; }

int vtableIndex(const char* vftableSymbol, const char* methodSymbol) {
    auto vt = reinterpret_cast<void**>(resolve(vftableSymbol));
    auto fn = resolve(methodSymbol);
    if (!vt || !fn) return -1;
    for (int i = 0; i < 512; i++) {
        if (vt[i] == fn) return i;
    }
    return -1;
}

// ---------------------------------------------------------------- CCDirector
void* director() { return call<void*>("?sharedDirector@CCDirector@cocos2d@@SAPEAV12@XZ"); }

void* runningScene() {
    auto d = director();
    if (!d) return nullptr;
    return call<void*>("?getRunningScene@CCDirector@cocos2d@@QEAAPEAVCCScene@2@XZ", d);
}

void* scheduler() {
    static int idx = vtableIndex("??_7CCDirector@cocos2d@@6BCCObject@1@@",
                                 "?getScheduler@CCDirector@cocos2d@@UEAAPEAVCCScheduler@2@XZ");
    auto d = director();
    if (!d) return nullptr;
    if (idx >= 0) return vcall<void*>(d, idx);
    return call<void*>("?getScheduler@CCDirector@cocos2d@@UEAAPEAVCCScheduler@2@XZ", d);
}

void* openGLView() {
    auto d = director();
    if (!d) return nullptr;
    return call<void*>("?getOpenGLView@CCDirector@cocos2d@@QEAAPEAVCCEGLView@2@XZ", d);
}

void* glfwWindow() {
    auto v = openGLView();
    if (!v) return nullptr;
    return call<void*>("?getWindow@CCEGLView@cocos2d@@QEBAPEAUGLFWwindow@@XZ", v);
}

Size winSize() {
    // metodo che restituisce una classe per valore: MSVC usa un puntatore nascosto dopo `this`
    Size out{0, 0};
    auto d = director();
    if (!d) return out;
    auto fn = reinterpret_cast<Size* (*)(void*, Size*)>(resolve("?getWinSize@CCDirector@cocos2d@@QEAA?AVCCSize@2@XZ"));
    if (fn) fn(d, &out);
    return out;
}

void setAnimationInterval(double interval) {
    // Il loop principale di GD (CCApplication::run) usa l'intervallo di CCApplication
    auto app = call<void*>("?sharedApplication@CCApplication@cocos2d@@SAPEAV12@XZ");
    if (app) call<void>("?setAnimationInterval@CCApplication@cocos2d@@UEAAXN@Z", app, interval);
}

double animationInterval() {
    auto d = director();
    if (!d) return 1.0 / 60.0;
    return call<double>("?getAnimationInterval@CCDirector@cocos2d@@QEAANXZ", d);
}

float deltaTime() {
    auto d = director();
    if (!d) return 0.f;
    return call<float>("?getDeltaTime@CCDirector@cocos2d@@UEAAMXZ", d);
}

// ---------------------------------------------------------------- CCScheduler
void setTimeScale(float s) {
    auto sc = scheduler();
    if (sc) call<void>("?setTimeScale@CCScheduler@cocos2d@@QEAAXM@Z", sc, s);
}

float timeScale() {
    auto sc = scheduler();
    if (!sc) return 1.f;
    return call<float>("?getTimeScale@CCScheduler@cocos2d@@QEAAMXZ", sc);
}

// ---------------------------------------------------------------- CCNode
static int nodeIdx(const char* method) { return vtableIndex("??_7CCNode@cocos2d@@6B@", method); }

void setVisible(void* node, bool v) {
    static int idx = nodeIdx("?setVisible@CCNode@cocos2d@@UEAAX_N@Z");
    if (!node) return;
    if (idx >= 0) vcall<void>(node, idx, v);
    else call<void>("?setVisible@CCNode@cocos2d@@UEAAX_N@Z", node, v);
}

bool isVisible(void* node) {
    static int idx = nodeIdx("?isVisible@CCNode@cocos2d@@UEAA_NXZ");
    if (!node) return false;
    if (idx >= 0) return vcall<bool>(node, idx);
    return call<bool>("?isVisible@CCNode@cocos2d@@UEAA_NXZ", node);
}

void* children(void* node) {
    if (!node) return nullptr;
    return call<void*>("?getChildren@CCNode@cocos2d@@UEAAPEAVCCArray@2@XZ", node);
}

void* parent(void* node) {
    if (!node) return nullptr;
    return call<void*>("?getParent@CCNode@cocos2d@@UEAAPEAV12@XZ", node);
}

Point position(void* node) {
    if (!node) return {0, 0};
    static int idx = nodeIdx("?getPosition@CCNode@cocos2d@@UEAAAEBVCCPoint@2@XZ");
    const Point* p = idx >= 0 ? vcall<const Point*>(node, idx)
                              : call<const Point*>("?getPosition@CCNode@cocos2d@@UEAAAEBVCCPoint@2@XZ", node);
    return p ? *p : Point{0, 0};
}

void setPosition(void* node, Point p) {
    if (!node) return;
    static int idx = nodeIdx("?setPosition@CCNode@cocos2d@@UEAAXAEBVCCPoint@2@@Z");
    if (idx >= 0) vcall<void>(node, idx, static_cast<const Point*>(&p));
    else call<void>("?setPosition@CCNode@cocos2d@@UEAAXAEBVCCPoint@2@@Z", node, static_cast<const Point*>(&p));
}

float scale(void* node) {
    if (!node) return 1.f;
    static int idx = nodeIdx("?getScale@CCNode@cocos2d@@UEAAMXZ");
    return idx >= 0 ? vcall<float>(node, idx) : call<float>("?getScale@CCNode@cocos2d@@UEAAMXZ", node);
}

void setScale(void* node, float s) {
    if (!node) return;
    static int idx = nodeIdx("?setScale@CCNode@cocos2d@@UEAAXM@Z");
    if (idx >= 0) vcall<void>(node, idx, s);
    else call<void>("?setScale@CCNode@cocos2d@@UEAAXM@Z", node, s);
}

int zOrder(void* node) {
    if (!node) return 0;
    return call<int>("?getZOrder@CCNode@cocos2d@@UEAAHXZ", node);
}

void setZOrder(void* node, int z) {
    if (!node) return;
    static int idx = nodeIdx("?setZOrder@CCNode@cocos2d@@UEAAXH@Z");
    if (idx >= 0) vcall<void>(node, idx, z);
    else call<void>("?setZOrder@CCNode@cocos2d@@UEAAXH@Z", node, z);
}

void removeFromParent(void* node) {
    if (!node) return;
    static int idx = nodeIdx("?removeFromParent@CCNode@cocos2d@@UEAAXXZ");
    if (idx >= 0) vcall<void>(node, idx);
    else call<void>("?removeFromParent@CCNode@cocos2d@@UEAAXXZ", node);
}

int tag(void* node) {
    if (!node) return -1;
    return call<int>("?getTag@CCObject@cocos2d@@UEBAHXZ", node);
}

void setOpacity(void* node, uint8_t o) {
    // Solo per CCNodeRGBA/CCSprite/CCLayerColor diretti: usa l'export specifico
    if (!node) return;
    call<void>("?setOpacity@CCNodeRGBA@cocos2d@@UEAAXE@Z", node, o);
}

void setColor(void* node, Color3B c) {
    if (!node) return;
    call<void>("?setColor@CCNodeRGBA@cocos2d@@UEAAXAEBU_ccColor3B@2@@Z", node, static_cast<const Color3B*>(&c));
}

// ---------------------------------------------------------------- CCArray
unsigned arrayCount(void* arr) {
    if (!arr) return 0;
    return call<unsigned>("?count@CCArray@cocos2d@@QEBAIXZ", arr);
}

void* arrayAt(void* arr, unsigned i) {
    if (!arr) return nullptr;
    return call<void*>("?objectAtIndex@CCArray@cocos2d@@QEAAPEAVCCObject@2@I@Z", arr, i);
}

// ---------------------------------------------------------------- CCObject
void retain(void* o) {
    if (o) call<void>("?retain@CCObject@cocos2d@@QEAAXXZ", o);
}
void release(void* o) {
    if (o) call<void>("?release@CCObject@cocos2d@@QEAAXXZ", o);
}

// ---------------------------------------------------------------- particelle / streak
void stopSystem(void* ps) {
    if (ps) call<void>("?stopSystem@CCParticleSystem@cocos2d@@QEAAXXZ", ps);
}
void resetSystem(void* ps) {
    if (ps) call<void>("?resetSystem@CCParticleSystem@cocos2d@@QEAAXXZ", ps);
}
void streakReset(void* streak) {
    if (streak) call<void>("?reset@CCMotionStreak@cocos2d@@QEAAXXZ", streak);
}

// ---------------------------------------------------------------- transizioni
void* transitionCreate(int type, float duration, void* scene) {
    static const char* names[] = {
        "?create@CCTransitionFade@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionCrossFade@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionFadeBL@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionFadeDown@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionFadeTR@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionFadeUp@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionFlipAngular@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionFlipX@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionFlipY@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionJumpZoom@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionMoveInB@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionMoveInL@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionMoveInR@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionMoveInT@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        nullptr, // Page Turn (firma diversa, gestita sotto)
        "?create@CCTransitionProgressInOut@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionProgressOutIn@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionProgressRadialCW@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionProgressRadialCCW@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionRotoZoom@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionShrinkGrow@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionSlideInB@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionSlideInL@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionSlideInR@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionSlideInT@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionSplitCols@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionSplitRows@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionTurnOffTiles@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionZoomFlipAngular@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionZoomFlipX@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
        "?create@CCTransitionZoomFlipY@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z",
    };
    constexpr int n = static_cast<int>(sizeof(names) / sizeof(names[0]));
    if (type < 0 || type >= n) type = 0;
    if (type == 14) {
        return call<void*>("?create@CCTransitionPageTurn@cocos2d@@SAPEAV12@MPEAVCCScene@2@_N@Z", duration, scene, false);
    }
    return call<void*>(names[type], duration, scene);
}

} // namespace cc
