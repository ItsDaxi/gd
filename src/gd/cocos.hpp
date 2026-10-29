#pragma once
// Wrapper minimi per libcocos2d.dll, risolti tramite i nomi esportati (MSVC mangled).
// L'elenco completo degli export e' in tools/cocos_exports.txt.

#include <cstdint>

namespace cc {

// Stesso ABI di cocos2d::CCPoint: il costruttore di copia definito dall'utente
// cambia il modo in cui MSVC passa la struttura per valore, quindi va replicato.
struct Point {
    float x = 0, y = 0;
    constexpr Point() = default;
    constexpr Point(float x_, float y_) : x(x_), y(y_) {}
    constexpr Point(const Point& o) : x(o.x), y(o.y) {}
    constexpr Point& operator=(const Point& o) {
        x = o.x;
        y = o.y;
        return *this;
    }
};
struct Size { float width, height; };
struct Color3B { uint8_t r, g, b; };
struct Color4F { float r, g, b, a; };

void* resolve(const char* symbol);

// Chiama un export: cc::call<R>("?simbolo", args...)
template <class R, class... A>
inline R call(const char* symbol, A... args) {
    using Fn = R (*)(A...);
    auto fn = reinterpret_cast<Fn>(resolve(symbol));
    if (!fn) return R();
    return fn(args...);
}

// Indice nella vtable di un metodo virtuale (cercando l'export dentro la vftable esportata)
int vtableIndex(const char* vftableSymbol, const char* methodSymbol);

// Chiamata virtuale tramite indice (rispetta gli override delle sottoclassi, es. CCSprite::setVisible)
template <class R, class... A>
inline R vcall(void* obj, int index, A... args) {
    if (!obj || index < 0) return R();
    auto vt = *reinterpret_cast<void***>(obj);
    using Fn = R (*)(void*, A...);
    return reinterpret_cast<Fn>(vt[index])(obj, args...);
}

// --- CCDirector ---
void* director();
void* runningScene();
void* scheduler();
void* openGLView();
Size winSize();
void setAnimationInterval(double interval);
double animationInterval();
float deltaTime();
void* glfwWindow();
void showCursor(bool show);   // CCEGLView::showCursor
void lockCursor(bool lock);   // CCEGLView::toggleLockCursor

// --- CCScheduler ---
void setTimeScale(float s);
float timeScale();

// --- CCNode ---
void setVisible(void* node, bool v);
bool isVisible(void* node);
void* children(void* node);          // CCArray*
void* parent(void* node);
Point position(void* node);
void setPosition(void* node, Point p);
float scale(void* node);
float rotation(void* node);
void setRotation(void* node, float r);
void setScale(void* node, float s);
int zOrder(void* node);
void setZOrder(void* node, int z);
void removeFromParent(void* node);
int tag(void* node);
void addChild(void* parent, void* child, int z);
void* nodeCreate();                 // CCNode::create

// --- CCDrawNode ---
void* drawNodeCreate();
void drawNodeClear(void* node);
void drawSegment(void* node, Point a, Point b, float radius, Color4F color);

// --- CCRGBAProtocol (interfaccia secondaria: serve il sotto-oggetto corretto) ---
void setOpacity(void* sprite, uint8_t o); // per CCSprite e derivati (GameObject, ecc.)
void setColor(void* sprite, Color3B c);

// --- CCLabelBMFont ---
void labelSetString(void* label, const char* text);
const char* labelGetString(void* label);

// --- CCArray ---
unsigned arrayCount(void* arr);
void* arrayAt(void* arr, unsigned i);

// --- CCObject ---
void retain(void* o);
void release(void* o);

// --- CCParticleSystem ---
void stopSystem(void* ps);
void resetSystem(void* ps);

// --- CCMotionStreak ---
void streakReset(void* streak);

// --- CCTransitionFade & co. ---
void* transitionCreate(int type, float duration, void* scene);

// --- RTTI MSVC: true se l'oggetto e' della classe indicata o la eredita
// (nome decorato del tipo, es. ".?AVCCSprite@cocos2d@@")
bool isKindOf(void* obj, const char* rttiName);

// --- Clipboard / misc ---
bool hasExport(const char* symbol);

} // namespace cc
