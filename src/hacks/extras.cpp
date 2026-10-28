// Funzioni aggiunte in un secondo momento (prima segnate come non disponibili)
#include "common.hpp"

#include <windows.h>

// StartPos create a runtime (condivise con lo StartPos Switcher di level.cpp)
namespace mh {
extern std::vector<void*> g_liveStartPos;
}

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

// Ball Rotation Bug: la palla compie un giro in piu' ad ogni rotazione, come il "barrel roll" della 2.0
Hack& ballRotationBug = hack("COSMETIC", "COSMETIC/HACK_BALL_ROTATION_BUG")
    .partial("Ricostruzione approssimativa: la rotazione della palla viene raddoppiata");
void (*o_ballRotation)(void*, float);
void h_ballRotation(void* self, float speed) {
    o_ballRotation(self, ballRotationBug.enabled ? speed * 2.f : speed);
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

// Compact Lists: forza la vista compatta che GD usa gia' per alcune liste (LevelCell::m_compactView,
// CommentCell::m_compactMode) e l'altezza di cella della variante compatta
Hack& compactLists = hack("UNIVERSAL", "UNIVERSAL/HACK_COMPACT_LISTS")
    .partial("Usa la vista compatta interna di GD per livelli e commenti; riapri la lista dopo averla attivata");
constexpr int kListLevel = 4, kListLevel2 = 5;
float (*o_cellHeight)(int);
float h_cellHeight(int type) {
    float h = o_cellHeight(type);
    if (compactLists.enabled && type == kListLevel) {
        float compact = o_cellHeight(kListLevel2);
        if (compact > 0.f && compact < h) return compact;
    }
    return h;
}
void (*o_loadFromLevel)(void*, void*);
void h_loadFromLevel(void* self, void* level) {
    if (compactLists.enabled && GDF(self, TableViewCell, m_listType, int) == kListLevel) GDF(self, LevelCell, m_compactView, bool) = true;
    o_loadFromLevel(self, level);
}
void (*o_loadFromComment)(void*, void*);
void h_loadFromComment(void* self, void* comment) {
    if (compactLists.enabled) GDF(self, CommentCell, m_compactMode, bool) = true;
    o_loadFromComment(self, comment);
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

std::vector<Portal> collectPortals(void* pl, std::vector<void*>* starts) {
    std::vector<Portal> portals;
    forEach(GDF(pl, GJBaseGameLayer, m_objects, void*), [&](void* o) {
        if (!o) return;
        int id = GDF(o, GameObject, m_objectID, int);
        double x = GDF(o, GameObject, m_positionX, double);
        if (id == 31) {
            if (starts) starts->push_back(o);
        } else if (gamemodeFor(id) >= 0 || speedFor(id) >= 0 || id == 99 || id == 101 || id == 45 || id == 46 || id == 286 ||
                   id == 287) {
            portals.push_back({x, id});
        }
    });
    std::sort(portals.begin(), portals.end(), [](const Portal& a, const Portal& b) { return a.x < b.x; });
    return portals;
}

void applySettingsTo(void* sp, const std::vector<Portal>& portals, bool legacy) {
    void* settings = GDF(sp, StartPosObject, m_startSettings, void*);
    if (!settings) return;
    double sx = GDF(sp, GameObject, m_positionX, double);
    int mode = -1, speed = -1, mini = -1, mirror = -1, dual = -1;
    for (auto& p : portals) {
        // legacy: considera anche i portali sovrapposti alla StartPos
        if (legacy ? p.x > sx : p.x >= sx) break;
        if (gamemodeFor(p.id) >= 0) mode = gamemodeFor(p.id);
        if (speedFor(p.id) >= 0) speed = speedFor(p.id);
        if (p.id == 99) mini = 0;
        if (p.id == 101) mini = 1;
        if (p.id == 45) mirror = 1;
        if (p.id == 46) mirror = 0;
        if (p.id == 286) dual = 1;
        if (p.id == 287) dual = 0;
    }
    namespace lso = gdg::off_unchecked::LevelSettingsObject;
    if (s_ssGm && mode >= 0) mem::at<int>(settings, lso::m_startMode) = mode;
    if (s_ssSpeed && speed >= 0) mem::at<int>(settings, lso::m_startSpeed) = speed;
    if (s_ssSize && mini >= 0) mem::at<bool>(settings, lso::m_startMini) = mini == 1;
    if (s_ssMirror && mirror >= 0) mem::at<bool>(settings, lso::m_mirrorMode) = mirror == 1;
    if (s_ssGm && dual >= 0) mem::at<bool>(settings, lso::m_startDual) = dual == 1;
}

void applySmartStartPos(void* pl) {
    std::vector<void*> starts;
    auto portals = collectPortals(pl, &starts);
    for (void* sp : starts) applySettingsTo(sp, portals, s_ssLegacy);
}

// Auto StartPos: crea una StartPos subito dopo ogni portale di modalita' (usando la funzione
// di GD 2.2 per le StartPos create dal menu di pausa) e la configura come Smart StartPos
Hack& autoStartPos = hack("LEVEL", "LEVEL/HACK_AUTO_START_POS").isCheat()
    .partial("Usa PlayLayer::addLiveStartPosObject; le StartPos create compaiono nello StartPos Switcher");

void createAutoStartPos(void* pl) {
    auto portals = collectPortals(pl, nullptr);
    void* previous = GDF(pl, GJBaseGameLayer, m_startPosObject, void*);
    for (auto& p : portals) {
        if (gamemodeFor(p.id) < 0 && p.id != 286 && p.id != 287) continue;
        void* sp = gd::call<void*>(gd::addr::GameObject::createWithKey, 31);
        if (!sp || GDF(sp, GameObject, m_objectID, int) != 31) continue;
        cc::retain(sp);
        double y = 105.0;
        forEach(GDF(pl, GJBaseGameLayer, m_objects, void*), [&](void* o) {
            if (o && GDF(o, GameObject, m_objectID, int) == p.id && GDF(o, GameObject, m_positionX, double) == p.x)
                y = GDF(o, GameObject, m_positionY, double);
        });
        double x = p.x + 5.0;
        cc::setPosition(sp, cc::Point(static_cast<float>(x), static_cast<float>(y)));
        GDF(sp, GameObject, m_positionX, double) = x;
        GDF(sp, GameObject, m_positionY, double) = y;
        gd::call<void>(gd::addr::PlayLayer::addLiveStartPosObject, pl, sp);
        applySettingsTo(sp, portals, true);
        g_liveStartPos.push_back(sp);
    }
    // addLiveStartPosObject puo' selezionare la StartPos appena creata: torna a quella di prima
    GDF(pl, GJBaseGameLayer, m_startPosObject, void*) = previous;
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
    if (autoStartPos.enabled) createAutoStartPos(pl);
});
MH_ON(ev::plQuit, [](void*) {
    for (void* sp : g_liveStartPos) cc::release(sp);
    g_liveStartPos.clear();
});

// =================================================================== COSMETIC: Classic Pulse
namespace {
Hack& classicPulse = hack("COSMETIC", "COSMETIC/HACK_CLASSIC_PULSE")
    .partial("Calcola la pulsazione ad ogni frame dal metering FMOD della musica (invece che a 60 Hz)");

struct MeteringInfo {
    int numsamples;
    float peaklevel[32];
    float rmslevel[32];
    short numchannels;
};
} // namespace

MH_EVERY_FRAME(classicPulse) {
    if (!classicPulse.enabled || !gd::playLayer()) return;
    auto f = gd::fmod();
    if (!f) return;
    void* group = GDF(f, FMODAudioEngine, m_backgroundMusicChannel, void*);
    static HMODULE fm = GetModuleHandleW(L"fmod.dll");
    if (!group || !fm) return;
    using GetDSP = int (*)(void*, int, void**);
    using SetMetering = int (*)(void*, bool, bool);
    using GetMetering = int (*)(void*, MeteringInfo*, MeteringInfo*);
    static auto getDSP = reinterpret_cast<GetDSP>(GetProcAddress(fm, "?getDSP@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@HPEAPEAVDSP@2@@Z"));
    static auto setMetering = reinterpret_cast<SetMetering>(GetProcAddress(fm, "?setMeteringEnabled@DSP@FMOD@@QEAA?AW4FMOD_RESULT@@_N0@Z"));
    static auto getMetering = reinterpret_cast<GetMetering>(GetProcAddress(fm, "?getMeteringInfo@DSP@FMOD@@QEAA?AW4FMOD_RESULT@@PEAUFMOD_DSP_METERING_INFO@@0@Z"));
    if (!getDSP || !setMetering || !getMetering) return;
    void* dsp = nullptr;
    if (getDSP(group, -1 /* FMOD_CHANNELCONTROL_DSP_HEAD */, &dsp) != 0 || !dsp) return;
    static void* enabledOn = nullptr;
    if (enabledOn != dsp) {
        setMetering(dsp, false, true);
        enabledOn = dsp;
    }
    MeteringInfo out{};
    if (getMetering(dsp, nullptr, &out) != 0) return;
    float peak = 0.f, rms = 0.f;
    int n = std::clamp<int>(out.numchannels, 0, 32);
    for (int i = 0; i < n; i++) {
        peak = std::max(peak, out.peaklevel[i]);
        rms = std::max(rms, out.rmslevel[i]);
    }
    float& p1 = GDF(f, FMODAudioEngine, m_pulse1, float);
    float& p2 = GDF(f, FMODAudioEngine, m_pulse2, float);
    float& p3 = GDF(f, FMODAudioEngine, m_pulse3, float);
    p1 = peak;
    p2 = p2 * 0.8f + peak * 0.2f;
    p3 = rms;
}

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
    hooks::gd("PlayerObject::runBallRotation", addr::PlayerObject::runBallRotation, h_ballRotation, &o_ballRotation);
    hooks::gd("EditorUI::onCreateObject", addr::EditorUI::onCreateObject, h_onCreateObject, &o_onCreateObject);
    hooks::gd("LevelEditorLayer::objectsAtPosition", addr::LevelEditorLayer::objectsAtPosition, h_objectsAtPosition, &o_objectsAtPosition);
    hooks::gd("SongSelectNode::audioNext", addr::SongSelectNode::audioNext, h_audioNext, &o_audioNext);
    hooks::gd("SongSelectNode::audioPrevious", addr::SongSelectNode::audioPrevious, h_audioPrev, &o_audioPrev);
    hooks::gd("SliderTouchLogic::ccTouchMoved", addr::SliderTouchLogic::ccTouchMoved, h_sliderMoved, &o_sliderMoved);
    hooks::gd("OptionsLayer::musicSliderChanged", addr::OptionsLayer::musicSliderChanged, h_musicSlider, &o_musicSlider);
    hooks::gd("GJListLayer::init", addr::GJListLayer::init, h_listInit, &o_listInit);
    hooks::gd("CustomListView::getCellHeight", addr::CustomListView::getCellHeight, h_cellHeight, &o_cellHeight);
    hooks::gd("LevelCell::loadFromLevel", addr::LevelCell::loadFromLevel, h_loadFromLevel, &o_loadFromLevel);
    hooks::gd("CommentCell::loadFromComment", addr::CommentCell::loadFromComment, h_loadFromComment, &o_loadFromComment);
    hooks::exported("fmod.dll", "?createStream@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAUFMOD_CREATESOUNDEXINFO@@PEAPEAVSound@2@@Z",
                    h_createStream, &o_createStream);
}
