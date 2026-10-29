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
- **Respawn Time** ora funziona anche per accorciare il respawn: GD usa come ritardo max(1 s, tempo impostato), quindi prima si poteva solo allungarlo; ora viene cambiato il ritardo vero (`CCDelayTime` creato in `PlayLayer::destroyPlayer`)
- **No Wave Pulse**, **Wave Pulse Size** e il pulse di **Custom Wave Trail** ora hanno effetto: GD ricalcola il pulse e ridisegna la scia nello stesso update, prima del punto in cui il mod lo cambiava; ora viene cambiato subito prima del disegno (`HardStreak::updateStroke`)

**Verifiche sull'eseguibile di GD 2.2091** (disassemblato): tutte le 141 funzioni usate dal mod iniziano dove previsto. Correzioni trovate così:
- **Physics TPS**: GD calcola i passi con la costante 4 (= 240/60) in `GJBaseGameLayer::update`, non con 240; ora viene cambiata anche quella, prima il numero di tick non cambiava
- **Custom Object Bypass**: nella 2.2 i limiti sono 1000 oggetti per oggetto custom e 200 oggetti custom (non 50); ora vengono tolti davvero
- **Smooth Editor Trail**: usa il timer di GD (`m_trailTimer`, 1/30 s), così la linea è quella originale aggiornata a ogni frame
- **Multiple Editor Trails**: copia la linea di GD prima che il nuovo playtest la cancelli, con gli stessi colori (verde P1, azzurro P2)
- **Accurate Save**: corretta la Y (GD salva la posizione − 90)
- **Object Trajectory**: lo spostamento dei trigger move è già in punti, il percorso era 3 volte troppo lungo
- **Frame Extrapolation**: gestito anche il resto negativo del tick e il time warp

- **No Menu Animations** (finestra Universal): popup senza rimbalzo, menu a tendina e transizioni tra schermate istantanei. Riguarda i menu di GD, non il mod menu
- **Custom Wave Trail** (Cosmetic): colore fisso, arcobaleno o colori del giocatore, spessore, pulse (normale / nessuno / moltiplicatore / fisso), opacità, scia solida; anche per il giocatore 2. Il colore della wave trail è anche negli effetti di Iconic
- Funzioni prima non disponibili, ora presenti: Frozen Animations, Classic Particles, Classic Pulse, Ball Rotation Bug, Toolbox Button Bypass, Place Over, Default Song Bypass, Trigger Value Bypass, Custom Object Bypass, Slider Limit, Allow Low Volume, Transparent Lists, Compact Lists, Load Audio to Memory, Smart StartPos, Auto StartPos, Shipcopter, **Physics TPS**
- Physics TPS e Custom Object Bypass sono **sperimentali**: cercano a runtime nel codice di GD i punti da cambiare (1/240 e 240/60 per i tick, limiti 1000/200 degli oggetti custom). Il log (`megahack/log.txt`) dice quanti punti sono stati trovati
- Ora ci sono anche le ultime 6 funzioni che mancavano, quindi **nessuna è più non disponibile**:
  - **Show Trajectory** (Level): percorso del giocatore tenendo premuto (verde) o lasciando (arancione), con il punto di morte; simula la fisica di GD con due giocatori finti nascosti. Sperimentale
  - **Smooth Editor Trail** e **Multiple Editor Trails** (Creator): la linea verde del playtest aggiornata a ogni frame e le linee dei playtest precedenti, con trasparenza e i tasti Clear All / Latest / Oldest
  - **Object Trajectory** (Creator): percorso degli oggetti selezionati in base ai trigger move (non i rotate)
  - **Accurate Save** (Creator): salva X e Y degli oggetti con 4 decimali
  - **Frame Extrapolation** (Display): sopra i 240 fps sposta giocatori e camera della frazione di tick mancante

Elenco completo delle funzioni e del loro stato: [docs/FUNZIONI.md](https://github.com/ItsDaxi/gd/blob/claude/geometry-dash-mega-hack-mod-cj70zj/docs/FUNZIONI.md)
