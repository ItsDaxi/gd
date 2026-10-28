#pragma once
// Registro di tutte le funzioni (hack) e delle loro opzioni.
// Ogni hack usa come id la chiave di traduzione di Mega Hack
// (https://github.com/absolllute/Mega-Hack-Translate/blob/master/langs/en-GB.json)

#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace mh {

enum class OptType { Bool, Int, Float, Text, Combo, Key, Color, Button, Label };

struct Option {
    OptType type = OptType::Bool;
    std::string key;    // chiave nel config
    std::string label;  // chiave di traduzione (o testo)
    void* ptr = nullptr;
    float min = 0, max = 1;
    const char* fmt = nullptr;
    std::vector<std::string> items; // per Combo: chiavi di traduzione
    std::function<void()> onClick;  // per Button
    std::function<void()> onChange;
};

struct Hack {
    std::string id;      // chiave di traduzione, es. LEVEL/NOCLIP/HACK_NOCLIP
    std::string window;  // finestra, es. LEVEL
    bool enabled = false;
    bool cheat = false;      // conta come cheat (safe mode / cheat indicator)
    bool button = false;     // azione istantanea invece di un toggle
    bool notice = false;     // funzione disponibile solo in parte (vedi tip)
    std::string extraTip;    // nota aggiuntiva (es. limitazioni)
    std::vector<Option> options;
    std::function<void(bool)> onToggle;
    std::function<void()> onPress;     // per i bottoni
    std::function<void()> customUI;    // UI extra nel popup opzioni
    int keybind = 0;                   // virtual-key, 0 = nessuno
    std::string displayName;           // testo se non esiste la chiave di traduzione

    // --- builder ---
    Hack& isCheat(bool c = true) { cheat = c; return *this; }
    Hack& partial(std::string tip) { notice = true; extraTip = std::move(tip); return *this; }
    Hack& name(std::string n) { displayName = std::move(n); return *this; }
    Hack& toggled(std::function<void(bool)> f) { onToggle = std::move(f); return *this; }
    Hack& ui(std::function<void()> f) { customUI = std::move(f); return *this; }
    Hack& optBool(const std::string& label, bool* p, std::function<void()> changed = {});
    Hack& optInt(const std::string& label, int* p, int mn, int mx, const char* fmt = "%d");
    Hack& optFloat(const std::string& label, float* p, float mn, float mx, const char* fmt = "%.2f");
    Hack& optText(const std::string& label, std::string* p);
    Hack& optCombo(const std::string& label, int* p, std::vector<std::string> items);
    Hack& optKey(const std::string& label, int* p);
    Hack& optColor(const std::string& label, float* rgba4);
    Hack& optButton(const std::string& label, std::function<void()> f);

    void set(bool on);
    void toggle() { set(!enabled); }
};

// Crea (o restituisce) un hack. Le chiamate avvengono durante l'inizializzazione statica.
Hack& hack(const std::string& window, const std::string& id);
// Bottone (azione istantanea)
Hack& action(const std::string& window, const std::string& id, std::function<void()> f);

std::deque<Hack>& hacks();
Hack* find(const std::string& id);

// Ordine delle finestre (chiavi di traduzione)
const std::vector<std::string>& windowOrder();

// Installer degli hook: ogni file di hack ne registra uno
struct Installer {
    Installer(const char* name, void (*fn)());
};
void runInstallers();

// Callback per frame (chiamate dal thread principale dentro swapBuffers)
struct FrameCallback {
    explicit FrameCallback(void (*fn)());
};
void runFrameCallbacks();

// Chiama onToggle(true) per gli hack attivati dal config (primo frame)
void applyLoadedState();

// true se almeno un cheat e' attivo
bool anyCheatEnabled();
// Cheat usati durante il tentativo attuale (per Auto Safe Mode)
extern bool g_cheatedThisAttempt;

} // namespace mh

#define MH_CONCAT_(a, b) a##b
#define MH_CONCAT(a, b) MH_CONCAT_(a, b)
#define MH_INSTALL(name) \
    static void MH_CONCAT(mh_install_, name)(); \
    static ::mh::Installer MH_CONCAT(mh_installer_, name)(#name, &MH_CONCAT(mh_install_, name)); \
    static void MH_CONCAT(mh_install_, name)()
#define MH_EVERY_FRAME(name) \
    static void MH_CONCAT(mh_frame_, name)(); \
    static ::mh::FrameCallback MH_CONCAT(mh_framecb_, name)(&MH_CONCAT(mh_frame_, name)); \
    static void MH_CONCAT(mh_frame_, name)()
