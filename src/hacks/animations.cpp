// No Menu Animations: toglie le animazioni dei menu di Geometry Dash (non del mod menu)
#include "common.hpp"

using namespace mh;

namespace {

bool s_popups = true, s_dropdowns = true, s_transitions = true;
Hack& noMenuAnims = hack("UNIVERSAL", "UNIVERSAL/HACK_NO_MENU_ANIMATIONS").name("No Menu Animations")
    .optBool("Popups", &s_popups)
    .optBool("Drop-down Menus", &s_dropdowns)
    .optBool("Scene Transitions", &s_transitions);

// Popup (FLAlertLayer e derivati): niente "rimbalzo" elastico in apertura
void (*o_alertShow)(void*);
void h_alertShow(void* self) {
    bool skip = noMenuAnims.enabled && s_popups;
    if (skip) GDF(self, FLAlertLayer, m_noElasticity, bool) = true;
    o_alertShow(self);
    if (!skip) return;
    // ferma l'animazione di ingresso e porta subito il popup alla dimensione finale
    if (auto main = GDF(self, FLAlertLayer, m_mainLayer, void*)) {
        cc::call<void>("?stopAllActions@CCNode@cocos2d@@QEAAXXZ", main);
        cc::setScale(main, 1.f);
    }
}

// Menu a tendina (GJDropDownLayer: lista livelli, opzioni, ecc.): apertura/chiusura istantanea
void (*o_dropShow)(void*, bool);
void h_dropShow(void* self, bool instant) {
    o_dropShow(self, instant || (noMenuAnims.enabled && s_dropdowns));
}
void (*o_dropHide)(void*, bool);
void h_dropHide(void* self, bool instant) {
    o_dropHide(self, instant || (noMenuAnims.enabled && s_dropdowns));
}

} // namespace

// Transizioni tra le schermate: durata zero
MH_ON(ev::transition, [](float& duration, int& type) {
    if (noMenuAnims.enabled && s_transitions) {
        duration = 0.f;
        type = 0;
    }
});

MH_INSTALL(menuAnimations) {
    using namespace gd;
    hooks::gd("FLAlertLayer::show", addr::FLAlertLayer::show, h_alertShow, &o_alertShow);
    hooks::gd("GJDropDownLayer::showLayer", addr::GJDropDownLayer::showLayer, h_dropShow, &o_dropShow);
    hooks::gd("GJDropDownLayer::hideLayer", addr::GJDropDownLayer::hideLayer, h_dropHide, &o_dropHide);
}
