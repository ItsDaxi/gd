#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>

struct ImFont;

namespace mh {
struct Hack;
}

namespace mh::gui {

struct WindowState {
    float x = 0, y = 0;
    bool collapsed = false;
    bool userMoved = false; // spostata a mano: non viene piu' riposizionata dal layout automatico
    float maxH = 0;         // altezza massima assegnata dal layout (0 = schermo)
};

WindowState& windowState(const std::string& name);
std::map<std::string, WindowState>& windowStates();
std::vector<std::string>& favourites();
bool isFavourite(const std::string& id);

bool menuOpen();
void toggleMenu();
// Riporta tutte le finestre al layout predefinito (come Mega Hack)
void sortWindows();

void drawMenu();
void drawOverlays();

// Contenuto personalizzato per una finestra (es. STATUS, HOME, KEYBINDS)
// before=true: disegnato prima della lista degli hack
struct WindowContent {
    WindowContent(const char* window, void (*fn)(), bool before = false);
};

// Disegno sopra al gioco (overlay di stato, tint noclip, ecc.)
struct Overlay {
    explicit Overlay(void (*fn)());
};

// Gestore tasti (keybind speciali). Restituisce true per bloccare il tasto al gioco.
struct KeyHandler {
    explicit KeyHandler(bool (*fn)(int vk, bool down));
};
bool dispatchKey(int vk, bool down);

// Font grande per l'overlay
ImFont* bigFont();

// --- widget in stile Mega Hack: tutti a riga intera, alti come una riga del menu ---
float rowHeight();
bool hackRow(Hack& h);
bool checkboxRow(const char* label, bool* v);
bool buttonRow(const char* label, bool active = false);
// due bottoni affiancati: restituisce 1 o 2 se premuti, 0 altrimenti
int buttonPair(const char* a, const char* b, bool activeA = false, bool activeB = false);
void keyButton(const char* label, int* vk);
bool sliderIntRow(const char* id, int* v, int mn, int mx, const char* fmt);
bool sliderFloatRow(const char* id, float* v, float mn, float mx, const char* fmt);
bool inputRow(const char* id, const char* hint, std::string* value);
// combo a riga intera: usare come ImGui::BeginCombo (chiudere con ImGui::EndCombo)
bool beginComboRow(const char* id, const char* preview);
void labelRow(const char* text);
std::string keyName(int vk);

extern std::string g_search;

} // namespace mh::gui
