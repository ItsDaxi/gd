// Funzioni aggiunte in un secondo momento (prima segnate come non disponibili)
#include "common.hpp"

#include <windows.h>

using namespace mh;

namespace {

// =================================================================== COSMETIC
// Frozen Animations: ferma le animazioni a parti di Robot e Spider
Hack& frozenAnims = hack("COSMETIC", "COSMETIC/HACK_FROZEN_ANIMATIONS");

void freezeSprite(void* animated) {
    if (!animated) return;
    if (auto pa = GDF(animated, CCAnimatedSprite, m_paSprite, void*)) {
        cc::call<void>("?stopAllActions@CCNode@cocos2d@@QEAAXXZ", pa);
        forEach(cc::children(pa), [](void* part) { cc::call<void>("?stopAllActions@CCNode@cocos2d@@QEAAXXZ", part); });
    }
    cc::call<void>("?stopAllActions@CCNode@cocos2d@@QEAAXXZ", animated);
}

// Classic Particles: riattiva le particelle che seguivano il giocatore nella 2.1
Hack& classicParticles = hack("COSMETIC", "COSMETIC/HACK_CLASSIC_PARTICLES")
    .partial("Riattiva le particelle 'trailing' di PlayerObject: l'aspetto puo' differire dalla 2.1");

} // namespace

MH_ON(ev::plPostUpdate, [](void* pl, float) {
    for (auto p : {gd::player1(pl), gd::player2(pl)}) {
        if (!p) continue;
        if (frozenAnims.enabled) {
            freezeSprite(GDF(p, PlayerObject, m_robotSprite, void*));
            freezeSprite(GDF(p, PlayerObject, m_spiderSprite, void*));
        }
        if (classicParticles.enabled) {
            auto ps = GDF(p, PlayerObject, m_trailingParticles, void*);
            if (ps && !cc::isVisible(ps)) {
                cc::setVisible(ps, true);
                cc::resetSystem(ps);
            }
        }
    }
});

// =================================================================== CREATOR
namespace {

// Toolbox Button Bypass: righe/colonne dei pulsanti oggetto oltre i limiti del menu di GD
int s_toolCols = 6, s_toolRows = 2;
void applyToolbox() {
    if (!gd::gameManager()) return;
    gd::call<void>(gd::addr::GameManager::setIntGameVariable, gd::gameManager(), "0049", s_toolCols);
    gd::call<void>(gd::addr::GameManager::setIntGameVariable, gd::gameManager(), "0050", s_toolRows);
}
Hack& toolboxBypass = hack("CREATOR", "CREATOR/HACK_TOOLBOX_BUTTON_BYPASS")
    .optInt("Buttons per Row", &s_toolCols, 1, 40)
    .optInt("Button Rows", &s_toolRows, 1, 12)
    .toggled([](bool on) {
        if (on) applyToolbox();
    })
    .partial("Imposta le variabili 0049/0050 di GD: riapri l'editor per vedere il cambiamento");

// Place Over: durante il posizionamento l'editor non vede gli oggetti gia' presenti nel punto
Hack& placeOver = hack("CREATOR", "CREATOR/HACK_PLACE_OVER");
bool s_placing = false;
void (*o_onCreateObject)(void*, int);
void h_onCreateObject(void* self, int id) {
    s_placing = placeOver.enabled;
    o_onCreateObject(self, id);
    s_placing = false;
}
void* (*o_objectsAtPosition)(void*, cc::Point);
void* h_objectsAtPosition(void* self, cc::Point p) {
    if (s_placing) return cc::call<void*>("?create@CCArray@cocos2d@@SAPEAV12@XZ");
    return o_objectsAtPosition(self, p);
}

// Default Song Bypass: le frecce delle canzoni ufficiali vanno oltre l'ultima/prima
Hack& defaultSongBypass = hack("CREATOR", "CREATOR/HACK_DEFAULT_SONG_BYPASS");
void setSong(void* node, int id) {
    GDF(node, SongSelectNode, m_selectedSongID, int) = id;
    if (auto settings = GDF(node, SongSelectNode, m_settingsObject, void*)) {
        if (auto level = mem::at<void*>(settings, gdg::off_unchecked::LevelSettingsObject::m_level)) GDF(level, GJGameLevel, m_audioTrack, int) = id;
    }
    gd::call<void>(gd::addr::SongSelectNode::updateAudioLabel, node);
}
void (*o_audioNext)(void*, void*);
void h_audioNext(void* self, void* sender) {
    int before = GDF(self, SongSelectNode, m_selectedSongID, int);
    o_audioNext(self, sender);
    if (defaultSongBypass.enabled && GDF(self, SongSelectNode, m_selectedSongID, int) == before) setSong(self, before + 1);
}
void (*o_audioPrev)(void*, void*);
void h_audioPrev(void* self, void* sender) {
    int before = GDF(self, SongSelectNode, m_selectedSongID, int);
    o_audioPrev(self, sender);
    if (defaultSongBypass.enabled && GDF(self, SongSelectNode, m_selectedSongID, int) == before && before > 0)
        setSong(self, before - 1);
}

} // namespace

MH_ON(ev::editorInit, [](void*) {
    if (toolboxBypass.enabled) applyToolbox();
});

// =================================================================== BYPASS
namespace {

// Slider Limit: dopo il trascinamento normale sposta il cursore anche oltre la barra
Hack& sliderLimit = hack("BYPASS", "BYPASS/HACK_SLIDER_LIMIT");
void (*o_sliderMoved)(void*, void*, void*);
void h_sliderMoved(void* self, void* touch, void* event) {
    o_sliderMoved(self, touch, event);
    if (!sliderLimit.enabled || !touch) return;
    void* thumb = GDF(self, SliderTouchLogic, m_thumb, void*);
    void* parent = thumb ? cc::parent(thumb) : nullptr;
    float length = GDF(self, SliderTouchLogic, m_length, float);
    if (!thumb || !parent || length <= 0.f) return;
    // CCTouch::getLocation e CCNode::convertToNodeSpace restituiscono CCPoint: puntatore nascosto
    cc::Point world, local;
    auto getLoc = reinterpret_cast<cc::Point* (*)(void*, cc::Point*)>(cc::resolve("?getLocation@CCTouch@cocos2d@@QEBA?AVCCPoint@2@XZ"));
    auto toNode = reinterpret_cast<cc::Point* (*)(void*, cc::Point*, const cc::Point*)>(
        cc::resolve("?convertToNodeSpace@CCNode@cocos2d@@QEAA?AVCCPoint@2@AEBV32@@Z"));
    if (!getLoc || !toNode) return;
    getLoc(touch, &world);
    toNode(parent, &local, &world);
    bool vertical = GDF(self, SliderTouchLogic, m_rotated, bool);
    float v = vertical ? local.y : local.x;
    float half = length * 0.5f;
    if (v >= -half && v <= half) return; // dentro la barra: ci pensa GD
    cc::Point pos = cc::position(thumb);
    if (vertical) pos.y = v;
    else pos.x = v;
    cc::setPosition(thumb, pos);
    cc::call<void>("?activate@CCMenuItem@cocos2d@@UEAAXXZ", thumb); // richiama il callback dello slider
}

} // namespace

// =================================================================== UNIVERSAL
namespace {

// Allow Low Volume: GD azzera il volume della musica sotto il 3%
Hack& allowLowVolume = hack("UNIVERSAL", "UNIVERSAL/HACK_ALLOW_LOW_VOLUME");
void (*o_musicSlider)(void*, void*);
void h_musicSlider(void* self, void* sender) {
    o_musicSlider(self, sender);
    if (!allowLowVolume.enabled || !sender) return;
    float v = gd::call<float>(gd::addr::SliderThumb::getValue, sender);
    auto f = gd::fmod();
    if (!f || v <= 0.f || v >= 0.03f) return;
    GDF(f, FMODAudioEngine, m_musicVolume, float) = v;
    if (auto group = GDF(f, FMODAudioEngine, m_backgroundMusicChannel, void*)) {
        static HMODULE fm = GetModuleHandleW(L"fmod.dll");
        using SetVolume = int (*)(void*, float);
        auto setVolume = fm ? reinterpret_cast<SetVolume>(GetProcAddress(fm, "?setVolume@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@M@Z")) : nullptr;
        if (setVolume) setVolume(group, v);
    }
    if (auto gm = gd::gameManager()) GDF(gm, GameManager, m_bgVolume, float) = v;
}

// Transparent Lists: toglie lo sfondo colorato delle liste
Hack& transparentLists = hack("UNIVERSAL", "UNIVERSAL/HACK_TRANSPARENT_LISTS");
bool (*o_listInit)(void*, void*, const char*, uint32_t, float, float, int);
bool h_listInit(void* self, void* list, const char* title, uint32_t color, float w, float h, int type) {
    bool r = o_listInit(self, list, title, color, w, h, type);
    if (r && transparentLists.enabled) {
        // CCLayerColor: interfaccia RGBA dopo CCLayer (0x198)
        static int idx = cc::vtableIndex("??_7CCSprite@cocos2d@@6BCCRGBAProtocol@1@@", "?setOpacity@CCSprite@cocos2d@@UEAAXE@Z");
        auto sub = reinterpret_cast<char*>(self) + 0x198;
        auto vt = *reinterpret_cast<void***>(sub);
        if (idx >= 0 && mem::readable(vt, 8)) reinterpret_cast<void (*)(void*, uint8_t)>(vt[idx])(sub, 0);
    }
    return r;
}

// Load Audio to Memory: gli stream FMOD diventano suoni caricati in RAM
Hack& loadAudioMem = hack("UNIVERSAL", "UNIVERSAL/HACK_LOAD_AUDIO_TO_MEMORY");
using CreateFn = int (*)(void*, const char*, unsigned, void*, void**);
CreateFn o_createStream = nullptr;
int h_createStream(void* system, const char* name, unsigned mode, void* exinfo, void** sound) {
    static HMODULE fm = GetModuleHandleW(L"fmod.dll");
    static auto createSound = fm ? reinterpret_cast<CreateFn>(GetProcAddress(
                                       fm, "?createSound@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAUFMOD_CREATESOUNDEXINFO@@PEAPEAVSound@2@@Z"))
                                 : nullptr;
    constexpr unsigned FMOD_CREATESTREAM = 0x80, FMOD_CREATESAMPLE = 0x100;
    if (loadAudioMem.enabled && createSound) return createSound(system, name, (mode & ~FMOD_CREATESTREAM) | FMOD_CREATESAMPLE, exinfo, sound);
    return o_createStream(system, name, mode, exinfo, sound);
}

} // namespace

// =================================================================== LEVEL
namespace {

// Smart StartPos: imposta modalita', velocita', dimensione, specchio e dual di ogni StartPos
// in base ai portali che la precedono
bool s_ssLegacy = false, s_ssGm = true, s_ssSpeed = true, s_ssSize = true, s_ssMirror = true;
Hack& smartStartPos = hack("LEVEL", "LEVEL/SMART_STARTPOS/HACK_SMART_STARTPOS")
    .optBool("LEVEL/SMART_STARTPOS/DROPDOWN/LEGACY_CALCULATION", &s_ssLegacy)
    .optBool("LEVEL/SMART_STARTPOS/DROPDOWN/GAMEMODE_PORTALS", &s_ssGm)
    .optBool("LEVEL/SMART_STARTPOS/DROPDOWN/SPEED_PORTALS", &s_ssSpeed)
    .optBool("LEVEL/SMART_STARTPOS/DROPDOWN/SIZE_PORTALS", &s_ssSize)
    .optBool("LEVEL/SMART_STARTPOS/DROPDOWN/MIRROR_PORTALS", &s_ssMirror)
    .partial("Gli offset di LevelSettingsObject non sono validati dai size-check: verificare in gioco");

int gamemodeFor(int id) {
    switch (id) {
    case 12: return 0;   // cubo
    case 13: return 1;   // nave
    case 47: return 2;   // palla
    case 111: return 3;  // ufo
    case 660: return 4;  // onda
    case 745: return 5;  // robot
    case 1331: return 6; // ragno
    case 1933: return 7; // swing
    default: return -1;
    }
}
int speedFor(int id) {
    switch (id) {
    case 200: return 1;  // 0.5x
    case 201: return 0;  // 1x
    case 202: return 2;  // 2x
    case 203: return 3;  // 3x
    case 1334: return 4; // 4x
    default: return -1;
    }
}

struct Portal {
    double x;
    int id;
};

void applySmartStartPos(void* pl) {
    std::vector<Portal> portals;
    std::vector<void*> starts;
    forEach(GDF(pl, GJBaseGameLayer, m_objects, void*), [&](void* o) {
        if (!o) return;
        int id = GDF(o, GameObject, m_objectID, int);
        double x = GDF(o, GameObject, m_positionX, double);
        if (id == 31) starts.push_back(o);
        else if (gamemodeFor(id) >= 0 || speedFor(id) >= 0 || id == 99 || id == 101 || id == 45 || id == 46 || id == 286 || id == 287)
            portals.push_back({x, id});
    });
    std::sort(portals.begin(), portals.end(), [](const Portal& a, const Portal& b) { return a.x < b.x; });
    for (void* sp : starts) {
        void* settings = GDF(sp, StartPosObject, m_startSettings, void*);
        if (!settings) continue;
        double sx = GDF(sp, GameObject, m_positionX, double);
        int mode = -1, speed = -1, mini = -1, mirror = -1, dual = -1;
        for (auto& p : portals) {
            // legacy: considera anche i portali sovrapposti alla StartPos
            if (s_ssLegacy ? p.x > sx : p.x >= sx) break;
            if (gamemodeFor(p.id) >= 0) mode = gamemodeFor(p.id);
            if (speedFor(p.id) >= 0) speed = speedFor(p.id);
            if (p.id == 99) mini = 0;
            if (p.id == 101) mini = 1;
            if (p.id == 45) mirror = 1;
            if (p.id == 46) mirror = 0;
            if (p.id == 286) dual = 1;
            if (p.id == 287) dual = 0;
        }
        if (s_ssGm && mode >= 0) mem::at<int>(settings, gdg::off_unchecked::LevelSettingsObject::m_startMode) = mode;
        if (s_ssSpeed && speed >= 0) mem::at<int>(settings, gdg::off_unchecked::LevelSettingsObject::m_startSpeed) = speed;
        if (s_ssSize && mini >= 0) mem::at<bool>(settings, gdg::off_unchecked::LevelSettingsObject::m_startMini) = mini == 1;
        if (s_ssMirror && mirror >= 0) mem::at<bool>(settings, gdg::off_unchecked::LevelSettingsObject::m_mirrorMode) = mirror == 1;
        if (s_ssGm && dual >= 0) mem::at<bool>(settings, gdg::off_unchecked::LevelSettingsObject::m_startDual) = dual == 1;
    }
}

// Shipcopter: nello swing si sale tenendo premuto e si scende rilasciando (comandi della nave)
Hack& shipcopter = hack("LEVEL", "LEVEL/HACK_SHIPCOPTER").isCheat();

void setGravity(void* player, bool upsideDown) {
    auto layer = gd::gameLayer();
    if (!layer || GDF(player, PlayerObject, m_isUpsideDown, bool) == upsideDown) return;
    gd::call<void>(gd::addr::GJBaseGameLayer::flipGravity, layer, player, upsideDown, true);
}

} // namespace

MH_ON(ev::plInit, [](void* pl) {
    if (smartStartPos.enabled) applySmartStartPos(pl);
});

MH_ON(ev::poPushButton, [](void* player, int button, bool& allow) {
    if (!shipcopter.enabled || button != 1 || !GDF(player, PlayerObject, m_isSwing, bool)) return;
    // premere = gravita' verso l'alto; il flip normale dello swing non serve
    setGravity(player, true);
    allow = false;
});
MH_ON(ev::poReleaseButton, [](void* player, int button, bool& allow) {
    if (!shipcopter.enabled || button != 1 || !GDF(player, PlayerObject, m_isSwing, bool)) return;
    setGravity(player, false);
    allow = false;
});

MH_INSTALL(extras) {
    using namespace gd;
    hooks::gd("EditorUI::onCreateObject", addr::EditorUI::onCreateObject, h_onCreateObject, &o_onCreateObject);
    hooks::gd("LevelEditorLayer::objectsAtPosition", addr::LevelEditorLayer::objectsAtPosition, h_objectsAtPosition, &o_objectsAtPosition);
    hooks::gd("SongSelectNode::audioNext", addr::SongSelectNode::audioNext, h_audioNext, &o_audioNext);
    hooks::gd("SongSelectNode::audioPrevious", addr::SongSelectNode::audioPrevious, h_audioPrev, &o_audioPrev);
    hooks::gd("SliderTouchLogic::ccTouchMoved", addr::SliderTouchLogic::ccTouchMoved, h_sliderMoved, &o_sliderMoved);
    hooks::gd("OptionsLayer::musicSliderChanged", addr::OptionsLayer::musicSliderChanged, h_musicSlider, &o_musicSlider);
    hooks::gd("GJListLayer::init", addr::GJListLayer::init, h_listInit, &o_listInit);
    hooks::exported("fmod.dll", "?createStream@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAUFMOD_CREATESOUNDEXINFO@@PEAPEAVSound@2@@Z",
                    h_createStream, &o_createStream);
}
