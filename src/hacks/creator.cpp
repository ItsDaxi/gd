// Finestra CREATOR (editor)
#include "common.hpp"
#include "core/scan.hpp"

using namespace mh;

namespace {

Hack& H(const char* id) { return hack("CREATOR", id); }

Hack& accurateSave = H("CREATOR/HACK_ACCURATE_SAVE").partial("Non disponibile: la precisione e' nel formato di GameObject::getSaveString (serve una patch del binario)");
Hack& copyHack = H("CREATOR/HACK_COPY_HACK");
// Custom Object Bypass: il limite di 50 oggetti custom e' un "cmp reg, 50" dentro EditorUI::onNewCustomItem;
// lo si trova a runtime e lo si porta a 127 (massimo per un immediato a 8 bit)
std::vector<mem::Patch> s_customPatches;
Hack& customObjBypass = H("CREATOR/HACK_CUSTOM_OBJECT_BYPASS")
    .toggled([](bool on) {
        if (s_customPatches.empty()) {
            uintptr_t fn = gd::addr::EditorUI::onNewCustomItem ? mem::base() + gd::addr::EditorUI::onNewCustomItem : 0;
            if (fn) {
                for (int8_t limit : {int8_t(50), int8_t(49)})
                    for (uintptr_t a : scan::cmpImm8(fn, limit)) s_customPatches.emplace_back(a, std::vector<uint8_t>{0x7F});
            }
            MH_LOG("Custom Object Bypass: %zu limiti trovati", s_customPatches.size());
        }
        for (auto& p : s_customPatches) p.set(on);
    })
    .partial("Sperimentale: porta il limite a 127 oggetti custom (immediato trovato a runtime in EditorUI::onNewCustomItem)");
Hack& editorExtension = H("CREATOR/HACK_EDITOR_EXTENSION");
Hack& freeScroll = H("CREATOR/HACK_FREE_SCROLL");
Hack& hideUI = H("CREATOR/HACK_HIDE_UI").toggled([](bool on) {
    if (auto ui = gd::editorUI()) gd::call<void>(gd::addr::EditorUI::showUI, ui, !on);
});
Hack& levelEdit = H("CREATOR/HACK_LEVEL_EDIT");
Hack& noCMark = H("CREATOR/HACK_NO_C_MARK");
Hack& noPlaytestZoom = H("CREATOR/HACK_NO_PLAYTEST_ZOOM");
Hack& objectTrajectory = H("CREATOR/HACK_OBJECT_TRAJECTORY").partial("Non disponibile: richiede la simulazione dei trigger move/rotate");
Hack& smoothTrail = H("CREATOR/HACK_SMOOTH_EDITOR_TRAIL").partial("Non disponibile: LevelEditorLayer::addPlayerPoint e' inline su Windows (nessun indirizzo da agganciare)");
Hack& triggerValueBypass = H("CREATOR/HACK_TRIGGER_VALUE_BYPASS")
    .partial("Disattiva i limiti dei valori dei trigger (mappa m_shouldLimitValues di SetupTriggerPopup): "
             "i valori fuori scala si inseriscono dal campo di testo");

// std::map<int, bool> di MSVC: {nodo sentinella, dimensione}; ogni nodo e'
// {left, parent, right, color, isnil, pair<int,bool>} -> chiave a +0x1C, valore a +0x20
void clearLimitMap(void* map) {
    auto head = *reinterpret_cast<char**>(map);
    if (!head) return;
    std::vector<char*> stack;
    char* root = *reinterpret_cast<char**>(head + 8); // head->_Parent = radice
    if (root && root != head) stack.push_back(root);
    int guard = 0;
    while (!stack.empty() && guard++ < 100000) {
        char* n = stack.back();
        stack.pop_back();
        if (!n || n[0x19] /* _Isnil */) continue;
        n[0x20] = 0; // valore: false = nessun limite
        char* l = *reinterpret_cast<char**>(n);
        char* r = *reinterpret_cast<char**>(n + 16);
        if (l && !l[0x19]) stack.push_back(l);
        if (r && !r[0x19]) stack.push_back(r);
    }
}

void unlockTriggerPopup(void* popup) {
    clearLimitMap(&GDF(popup, SetupTriggerPopup, m_shouldLimitValues, char));
    clearLimitMap(&GDF(popup, SetupTriggerPopup, m_shouldLimitArrowValues, char));
}

void (*o_valueChanged)(void*, int, float);
void h_valueChanged(void* self, int prop, float v) {
    if (triggerValueBypass.enabled) unlockTriggerPopup(self);
    o_valueChanged(self, prop, v);
}
void (*o_updateValue)(void*, int, float);
void h_updateValue(void* self, int prop, float v) {
    if (triggerValueBypass.enabled) unlockTriggerPopup(self);
    o_updateValue(self, prop, v);
}
Hack& verifyHack = H("CREATOR/HACK_VERIFY_HACK");
float s_trailOpacity = 50.f;
Hack& multiTrails = H("CREATOR/MULTIPLE_EDITOR_TRAILS/HACK_MULTIPLE_EDITOR_TRAILS")
    .optFloat("CREATOR/MULTIPLE_EDITOR_TRAILS/DROPDOWN/TRANSPARENCY", &s_trailOpacity, 0, 100, "%.0f%%")
    .optButton("CREATOR/MULTIPLE_EDITOR_TRAILS/DROPDOWN/CLEAR_ALL", [] {})
    .optButton("CREATOR/MULTIPLE_EDITOR_TRAILS/DROPDOWN/CLEAR_LASTEST", [] {})
    .optButton("CREATOR/MULTIPLE_EDITOR_TRAILS/DROPDOWN/CLEAR_OLDEST", [] {})
    .partial("Non disponibile: LevelEditorLayer::clearPlayerPoints e' inline su Windows (nessun indirizzo da agganciare)");

// SeedValueRS: [random, seed], valore = random - seed
void setSeedRS(void* p, int v) {
    auto a = reinterpret_cast<int*>(p);
    a[0] = v + a[1];
}

void prepareUpload(void* level) {
    if (!level) return;
    if (verifyHack.enabled) {
        setSeedRS(&GDF(level, GJGameLevel, m_isVerified, char), 1);
        GDF(level, GJGameLevel, m_isVerifiedRaw, bool) = true;
    }
    if (noCMark.enabled) gd::setSeedValue(&GDF(level, GJGameLevel, m_originalLevel, char), 0);
}

// --- Verify Hack / No C Mark
bool (*o_shareInit)(void*, void*);
bool h_shareInit(void* self, void* level) {
    prepareUpload(level);
    return o_shareInit(self, level);
}
void (*o_editLevelShare)(void*, void*);
void h_editLevelShare(void* self, void* sender) {
    prepareUpload(GDF(self, EditLevelLayer, m_level, void*));
    o_editLevelShare(self, sender);
}

// --- Editor Extension: nessun limite di posizione (metodo che restituisce CCPoint: puntatore nascosto)
cc::Point* (*o_limitedPos)(void*, cc::Point*, cc::Point);
cc::Point* h_limitedPos(void* self, cc::Point* ret, cc::Point pos) {
    if (editorExtension.enabled) {
        *ret = pos;
        return ret;
    }
    return o_limitedPos(self, ret, pos);
}

// --- Free Scroll
void (*o_constrain)(void*, float, float);
void h_constrain(void* self, float x, float y) {
    if (freeScroll.enabled) return;
    o_constrain(self, x, y);
}

} // namespace

// --- No Playtest Zoom
MH_ON(ev::bglZoom, [](void* layer, float&, bool& allow) {
    if (noPlaytestZoom.enabled && layer == gd::editorLayer()) allow = false;
});

// --- Level Edit: il menu di pausa mostra "Edit" se il livello e' di tipo Editor (2)
namespace {
int s_savedType = -1;
void* s_editLevel = nullptr;
} // namespace
MH_ON(ev::pauseSetupPre, [](void*) {
    if (!levelEdit.enabled) return;
    auto lvl = gd::level();
    if (!lvl) return;
    s_editLevel = lvl;
    s_savedType = GDF(lvl, GJGameLevel, m_levelType, int);
    GDF(lvl, GJGameLevel, m_levelType, int) = 2;
});
MH_ON(ev::pauseSetup, [](void*) {
    if (s_editLevel && s_savedType >= 0) GDF(s_editLevel, GJGameLevel, m_levelType, int) = s_savedType;
    s_editLevel = nullptr;
    s_savedType = -1;
});

MH_ON(ev::editorInit, [](void* ui) {
    if (hideUI.enabled) gd::call<void>(gd::addr::EditorUI::showUI, ui, false);
});

MH_INSTALL(creator) {
    using namespace gd;
    hooks::gd("ShareLevelLayer::init", addr::ShareLevelLayer::init, h_shareInit, &o_shareInit);
    hooks::gd("EditLevelLayer::onShare", addr::EditLevelLayer::onShare, h_editLevelShare, &o_editLevelShare);
    hooks::gd("EditorUI::getLimitedPosition", addr::EditorUI::getLimitedPosition, h_limitedPos, &o_limitedPos);
    hooks::gd("SetupTriggerPopup::valueChanged", addr::SetupTriggerPopup::valueChanged, h_valueChanged, &o_valueChanged);
    hooks::gd("SetupTriggerPopup::updateValue", addr::SetupTriggerPopup::updateValue, h_updateValue, &o_updateValue);
    hooks::gd("EditorUI::constrainGameLayerPosition", addr::EditorUI::constrainGameLayerPosition, h_constrain, &o_constrain);
}

// LevelInfoLayer::init e' gia' agganciato in level.cpp (Auto Song Download): la Copy Hack
// si iscrive a quell'hook tramite questa funzione
namespace mh::creator {
void beforeLevelInfo(void* level) {
    if (copyHack.enabled && level) setSeedRS(&GDF(level, GJGameLevel, m_password, char), 1);
}
} // namespace mh::creator
