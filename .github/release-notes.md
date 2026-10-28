Mod menu in stile **Mega Hack** per Geometry Dash 2.2 su Windows, **senza Geode**.

> ⚠️ **Versione beta.** Il gioco si avvia e il menu si apre (hook 86/86 su 2.2091), ma molte funzioni non sono ancora state provate una per una. Se qualcosa non va, apri una issue allegando `megahack/log.txt`.

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
- Menu disposto a colonne come Mega Hack, finestre scorrevoli, controlli a riga intera
- Tab/Ins aprono e chiudono il menu senza navigare tra i campi
- In gioco, aprendo il menu, il cursore viene mostrato e sbloccato senza pausa
- Noclip: una morte per ogni ingresso in un ostacolo (prima contava ogni tick)
- Slider "Animations (ms)" funzionante con animazione di apertura

Elenco completo delle funzioni e del loro stato: [docs/FUNZIONI.md](https://github.com/ItsDaxi/gd/blob/claude/geometry-dash-mega-hack-mod-cj70zj/docs/FUNZIONI.md)
