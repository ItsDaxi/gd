Mod menu in stile **Mega Hack** per Geometry Dash 2.2 su Windows, **senza Geode**.

> ⚠️ **Versione beta.** Il gioco si avvia e il menu si apre, ma molte funzioni (soprattutto quelle nuove) non sono ancora state provate una per una. Se qualcosa non va, apri una issue allegando `megahack/log.txt`.

### Quale file scaricare
| Versione di GD (Settings → Options, in basso) | File |
|---|---|
| **2.2091** (consigliata) | `MegaHackGD-…-GD2.2091.zip` |
| 2.209 | `MegaHackGD-…-GD2.209.zip` (alcune funzioni disattivate: le bindings 2.209 sono incomplete) |

### Installazione
1. Estrai lo zip.
2. Copia **`MegaHackGD.dll`** e **`XInput1_4.dll`** nella cartella di Geometry Dash (Steam: tasto destro su GD → Gestisci → Sfoglia i file locali).
3. Avvia il gioco e premi **Tab** (o **Ins**) per aprire il menu.

Non usarlo insieme a Geode (usa lo stesso `XInput1_4.dll`). In alternativa al proxy puoi avviare `MegaHackInjector.exe` e poi il gioco.
Per disinstallare cancella i due file. Config, macro, screenshot e log stanno in `megahack/` nella cartella del gioco.

### Novità di questa versione
- **No Menu Animations** (finestra Universal): popup senza rimbalzo, menu a tendina e transizioni tra schermate istantanei. Riguarda i menu di GD, non il mod menu
- **Custom Wave Trail** (Cosmetic): colore fisso, arcobaleno o colori del giocatore, spessore, pulse (normale / nessuno / moltiplicatore / fisso), opacità, scia solida; anche per il giocatore 2. Il colore della wave trail è anche negli effetti di Iconic
- Funzioni prima non disponibili, ora presenti: Frozen Animations, Classic Particles, Classic Pulse, Ball Rotation Bug, Toolbox Button Bypass, Place Over, Default Song Bypass, Trigger Value Bypass, Custom Object Bypass, Slider Limit, Allow Low Volume, Transparent Lists, Compact Lists, Load Audio to Memory, Smart StartPos, Auto StartPos, Shipcopter, **Physics TPS**
- Physics TPS e Custom Object Bypass sono **sperimentali**: cercano a runtime nel codice di GD le costanti da cambiare (240 tick/s, limite 50). Il log (`megahack/log.txt`) dice quanti punti sono stati trovati
- Ora ci sono anche le ultime 6 funzioni che mancavano, quindi **nessuna è più non disponibile**:
  - **Show Trajectory** (Level): percorso del giocatore tenendo premuto (verde) o lasciando (arancione), con il punto di morte; simula la fisica di GD con due giocatori finti nascosti. Sperimentale
  - **Smooth Editor Trail** e **Multiple Editor Trails** (Creator): la linea verde del playtest aggiornata a ogni frame e le linee dei playtest precedenti, con trasparenza e i tasti Clear All / Latest / Oldest
  - **Object Trajectory** (Creator): percorso degli oggetti selezionati in base ai trigger move (non i rotate)
  - **Accurate Save** (Creator): salva X e Y degli oggetti con 4 decimali
  - **Frame Extrapolation** (Display): sopra i 240 fps sposta giocatori e camera della frazione di tick mancante

Elenco completo delle funzioni e del loro stato: [docs/FUNZIONI.md](https://github.com/ItsDaxi/gd/blob/claude/geometry-dash-mega-hack-mod-cj70zj/docs/FUNZIONI.md)
