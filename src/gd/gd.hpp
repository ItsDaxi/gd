#pragma once
// Accesso a Geometry Dash senza Geode.
// Indirizzi e offset vengono da src/gd/generated (generati da tools/broma_gen.py
// a partire dalle bindings pubbliche geode-sdk/bindings).

#include <cstdint>
#include <string>
#include <type_traits>

#include "core/memory.hpp"

#ifndef GD_VERSION
#define GD_VERSION 22091
#endif

#if GD_VERSION == 22091
#include "generated/gd_2_2091.hpp"
namespace gdg = gdv2_2091;
#define GD_VERSION_STR "2.2091"
#elif GD_VERSION == 2209
#include "generated/gd_2_209.hpp"
namespace gdg = gdv2_209;
#define GD_VERSION_STR "2.209"
#else
#error "GD_VERSION non supportata (usa 22091 o 2209)"
#endif

namespace gd {

namespace addr = gdg::addr;
namespace off = gdg::off;

// Chiama una funzione di GeometryDash.exe dato il suo RVA.
// x64 Windows: un'unica convenzione di chiamata, `this` e' il primo argomento.
// rva == 0: funzione non disponibile per questa versione -> nessuna chiamata
template <class R, class... A>
inline R call(uintptr_t rva, A... args) {
    if (rva == 0) {
        if constexpr (std::is_void_v<R>) return;
        else return R{};
    }
    return reinterpret_cast<R (*)(A...)>(mh::mem::base() + rva)(args...);
}

// Accesso ad un membro: GDF(oggetto, Classe, m_membro, tipo)
#define GDF(obj, Cls, member, T) ::mh::mem::at<T>((obj), ::gd::off::Cls::member)

// --- enum usati (da Geode/Enums.hpp delle bindings) ---
enum class GameObjectType : int {
    Solid = 0, Hazard = 2, InverseGravityPortal = 3, NormalGravityPortal = 4, ShipPortal = 5,
    CubePortal = 6, Decoration = 7, YellowJumpPad = 8, PinkJumpPad = 9, GravityPad = 10,
    YellowJumpRing = 11, PinkJumpRing = 12, GravityRing = 13, InverseMirrorPortal = 14,
    NormalMirrorPortal = 15, BallPortal = 16, RegularSizePortal = 17, MiniSizePortal = 18,
    UfoPortal = 19, Modifier = 20, Breakable = 21, SecretCoin = 22, DualPortal = 23,
    SoloPortal = 24, Slope = 25, WavePortal = 26, RobotPortal = 27, TeleportPortal = 28,
    GreenRing = 29, Collectible = 30, UserCoin = 31, DropRing = 32, SpiderPortal = 33,
    RedJumpPad = 34, RedJumpRing = 35, CustomRing = 36, DashRing = 37, GravityDashRing = 38,
    CollisionObject = 39, Special = 40, SwingPortal = 41, GravityTogglePortal = 42,
    SpiderOrb = 43, SpiderPad = 44, TeleportOrb = 46, AnimatedHazard = 47,
};

enum class PlayerButton : int { Jump = 1, Left = 2, Right = 3 };

enum class IconType : int {
    Cube = 0, Ship = 1, Ball = 2, Ufo = 3, Wave = 4, Robot = 5, Spider = 6, Swing = 7,
    Jetpack = 8, DeathEffect = 98, Special = 99, Item = 100, ShipFire = 101,
};

enum class UnlockType : int {
    Cube = 1, Col1 = 2, Col2 = 3, Ship = 4, Ball = 5, Bird = 6, Dart = 7, Robot = 8,
    Spider = 9, Streak = 10, Death = 11, GJItem = 12, Swing = 13, Jetpack = 14, ShipFire = 15,
};

// --- singleton / stato del gioco ---
void* gameManager();
void* gameStats();
void* fmod();
void* playLayer();     // PlayLayer corrente o nullptr
void* editorLayer();   // LevelEditorLayer corrente o nullptr
void* gameLayer();     // GJBaseGameLayer corrente (PlayLayer o editor) o nullptr
void* level();         // GJGameLevel del layer corrente
void* editorUI();      // EditorUI dell'editor aperto (catturato da EditorUI::init)

void* player1(void* layer);
void* player2(void* layer);

bool isPractice();
bool isPlatformer();
bool isDualMode();
float percent();       // percentuale attuale (0-100)
double levelTime();
int attempts();

// GJGameLevel helpers
int levelID(void* lvl);
std::string levelName(void* lvl);
int levelNormalPercent(void* lvl);

// std::string con il layout MSVC usato da GD. La memoria di GD va presa dall'heap del CRT
// dinamico (ucrtbase), non da quello statico di questa DLL.
std::string readStdString(const void* str);
bool writeStdString(void* str, const std::string& value);

// SeedValueRSV (valore offuscato di Geode: 3 int, il valore reale e' uno dei tre)
int seedValue(const void* p);
void setSeedValue(void* p, int v);

// game variables (GameManager::getGameVariable / setGameVariable)
bool getGameVar(const char* key);
void setGameVar(const char* key, bool v);

// Versione rilevata ok?
bool versionMatches();
const char* versionString();

} // namespace gd
