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
    bool placed = false;
};

WindowState& windowState(const std::string& name);
std::map<std::string, WindowState>& windowStates();
std::vector<std::string>& favourites();
bool isFavourite(const std::string& id);

bool menuOpen();
void toggleMenu();
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

// --- widget in stile Mega Hack, usabili dai file degli hack ---
bool hackRow(Hack& h);
bool checkboxRow(const char* label, bool* v);
bool buttonRow(const char* label);
void keyButton(const char* label, int* vk);
std::string keyName(int vk);

extern std::string g_search;

} // namespace mh::gui
