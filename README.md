# Mega Hack GD — mod menu standalone per Geometry Dash 2.209 / 2.2091 (senza Geode)

Mod menu in stile **Mega Hack** per Geometry Dash 2.2 su Windows, **senza Geode**: una DLL che si
carica da sola con il gioco e disegna il menu con Dear ImGui sopra la finestra di GD.

- Interfaccia come Mega Hack: finestre viola affiancate (Bypass, Cosmetic, Creator, Level, Universal,
  Status, Icons, Display, Speedhack, Shortcuts, Replay, Cheat Safety, Interface, Screenshot,
  Variables, Keybinds, Mega Hack, Favourites), righe che si accendono col colore d'accento, triangolino
  delle opzioni in basso a destra, ricerca, preferiti, keybind per ogni funzione, temi, scala, animazioni.
- **Tutte le 173 funzioni** elencate in
  [`en-GB.json` di Mega-Hack-Translate](https://github.com/absolllute/Mega-Hack-Translate/blob/master/langs/en-GB.json)
  sono presenti nel menu, più alcune extra della v8 viste nell'immagine di riferimento
  (Everything Hurts, Freeze Player, Insta Kill, Ignore ESC, Auto Practice Mode).
  L'elenco completo con lo stato di ognuna è in **[docs/FUNZIONI.md](docs/FUNZIONI.md)**.
- Testi e traduzioni presi direttamente dai file di Mega-Hack-Translate: inglese e **italiano** inclusi,
  altre lingue caricabili mettendo il loro `.json` in `megahack/langs/`.

> ⚠️ **Stato del progetto.** Il codice compila (MSVC in CI e MinGW) ma **non è ancora stato provato
> dentro il gioco**. Indirizzi e offset vengono dalle bindings pubbliche e sono verificati come
> descritto sotto, ma è normale che qualche funzione vada sistemata dopo i primi test. Se qualcosa non
> va, apri `megahack/log.txt` e guarda quali hook sono falliti.

## Installazione

1. Scarica l'artefatto della [CI](../../actions) per la tua versione di GD
   (`MegaHackGD-GD22091` per **2.2091**, `MegaHackGD-GD2209` per **2.209**), oppure compila (vedi sotto).
2. Copia **`MegaHackGD.dll`** e **`XInput1_4.dll`** nella cartella di Geometry Dash
   (quella con `GeometryDash.exe`, su Steam: *tasto destro → Gestisci → Sfoglia i file locali*).
3. Avvia il gioco e premi **Tab** (o **Ins**) per aprire il menu.

`XInput1_4.dll` è un *proxy*: GD la importa all'avvio, lei inoltra tutto alla vera DLL di Windows e
carica `MegaHackGD.dll`. **Non usarla insieme a Geode**, che usa lo stesso file.
In alternativa al proxy puoi usare **`MegaHackInjector.exe`** (avvialo, poi apri il gioco).

Per disinstallare basta cancellare i due file. Configurazione, macro, screenshot e log stanno in
`<cartella di GD>/megahack/`.

### Quale versione ho?

Le bindings di Geode coprono sia la **2.209** sia la hotfix **2.2091**: controlla il numero di versione
in basso nelle impostazioni di GD (*Settings → Options*) e scegli la DLL corrispondente. Se carichi la DLL
sbagliata, all'avvio il mod confronta gli indirizzi con l'eseguibile: se non combaciano mostra un avviso e
**disattiva gli hook di gioco** (il menu si apre comunque), così il gioco non va in crash.

## Come funziona (e perché senza Geode)

| Componente | Cosa fa |
|---|---|
| `tools/broma_gen.py` | Legge i file `.bro` di [geode-sdk/bindings](https://github.com/geode-sdk/bindings) (2.209 e 2.2091) e genera `src/gd/generated/*.hpp` con gli **indirizzi Windows** delle funzioni e gli **offset dei membri**, calcolati con le regole di layout di MSVC x64. |
| Validazione layout | Il layout calcolato viene confrontato con i `GEODE_SIZE_CHECK` ufficiali (`test/members/2.2091/Windows.cpp`): per 2.2091 combaciano 450 classi, tra cui `PlayLayer`, `PlayerObject`, `GameObject`, `GameManager`, `EditorUI`, `GJGameLevel`, `FMODAudioEngine`, `HardStreak`. Le classi che non combaciano vanno in `off_unchecked` e non vengono usate. |
| `tools/check_hooks.py` | Controlla che ogni funzione agganciata abbia un indirizzo **unico** (gli stub condivisi tipo `return false` non vanno agganciati). 2.2091: 80 hook, 0 problemi. |
| cocos2d | Le funzioni di `libcocos2d.dll` sono chiamate per **nome esportato** (`GetProcAddress` con il nome MSVC decorato, elenco in `tools/cocos_exports.txt`); i metodi virtuali passano dalla vtable reale dell'oggetto. |
| Hook | [MinHook](https://github.com/TsudaKageyu/minhook). Ogni funzione di gioco è agganciata **una sola volta** in `src/hacks/core_hooks.cpp`, che emette eventi (`src/hacks/events.hpp`) a cui si iscrivono le funzioni del menu. |
| Grafica | Hook di `CCEGLView::swapBuffers` → Dear ImGui (Win32 + OpenGL3). La finestra di GLFW viene sottoclassata per l'input: quando il mouse è sopra il menu il gioco non riceve i click. |

### Note sulla 2.209

Le bindings della 2.209 sono meno complete: alcuni indirizzi mancano o sono segnaposto e non esistono
size-check per validare gli offset. Il generatore mette a `0` gli indirizzi mancanti/condivisi
(`--names-from 2.2091 --zero-shared`) e il mod salta quegli hook. Su 2.209 quindi alcune funzioni
non sono attive e gli offset non sono validati: **se puoi, usa 2.2091**.

## Compilare

Serve Windows con Visual Studio 2022 (C++ desktop) e CMake ≥ 3.21. Le dipendenze (Dear ImGui, MinHook,
nlohmann/json) vengono scaricate da CMake.

```bash
cmake -S . -B build -A x64 -DGD_VERSION=22091
```

```bash
cmake --build build --config Release
```

Usa `-DGD_VERSION=2209` per GD 2.209. I file escono in `build/Release/`.

### Aggiornare gli indirizzi per una nuova versione di GD

```bash
git clone --depth 1 https://github.com/geode-sdk/bindings.git ../bindings
```

```bash
python3 tools/broma_gen.py ../bindings 2.2091 src/gd/generated/gd_2_2091.hpp --classes-file tools/classes.txt --verbose
```

```bash
python3 tools/check_hooks.py src/gd/generated/gd_2_2091.hpp
```

Per una versione nuova aggiungi il blocco corrispondente in `src/gd/gd.hpp` e in `CMakeLists.txt`.
`python3 tools/gen_features.py` rigenera `docs/FUNZIONI.md`.

## Uso

- **Tab / Ins**: apre/chiude il menu (modificabili nella finestra *Mega Hack*).
- **Click** su una riga: attiva/disattiva. **Click sul triangolino o tasto destro**: opzioni, keybind,
  aggiungi ai preferiti.
- Le funzioni segnate come cheat (Noclip, Speedhack, Show Hitboxes…) attivano il *Cheat Indicator* e,
  con **Safe Mode / Auto Safe Mode**, mettono il livello in *test mode* (come con le StartPos) così
  i progressi non vengono salvati.
- I preferiti si aggiungono dal popup delle opzioni e compaiono nella finestra *Favourites*.

## Crediti

- Elenco funzioni e testi: [absolllute/Mega-Hack-Translate](https://github.com/absolllute/Mega-Hack-Translate)
  (Mega Hack è di Absolute; questo progetto non è affiliato).
- Indirizzi e layout: [geode-sdk/bindings](https://github.com/geode-sdk/bindings) (usate solo come dati, senza Geode).
- [Dear ImGui](https://github.com/ocornut/imgui) (MIT), [MinHook](https://github.com/TsudaKageyu/minhook) (BSD-2),
  [nlohmann/json](https://github.com/nlohmann/json) (MIT).

Usa le funzioni cheat in modo responsabile: non caricare record ottenuti con noclip/speedhack/macro
nelle classifiche o nelle demon list.
