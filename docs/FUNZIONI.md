# Elenco funzioni

Generato da `tools/gen_features.py` leggendo il codice sorgente. Nomi e descrizioni vengono da [Mega-Hack-Translate](https://github.com/absolllute/Mega-Hack-Translate) (`en-GB.json` / `it-IT.json`).

Legenda stato: ✅ implementata (compilata, non ancora provata in gioco) · ⚠️ parziale / con limitazioni · ❌ non disponibile senza Geode o senza patch del binario · 🔴 = conta come cheat (Safe Mode / Cheat Indicator)

**Totale:** 198 funzioni — ✅ 163 · ⚠️ 29 · ❌ 6

## Bypass

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Anti-Kick | Anti-espulsione | ✅ | Prevents you being blocked from playing certain levels |
| Challenge Level |  | ⚠️ | Sblocca tramite le variabili di gioco (UGV) e le statistiche |
| Keymaster |  | ⚠️ | Sblocca tramite le variabili di gioco (UGV) e le statistiche |
| Main Levels | Livelli Principali | ✅ | Unlocks main demon levels |
| Music Customiser | Customizer Musica | ✅ | Allows for custom practice and menu music |
| Slider Limit | Limite Slider | ✅ | Let sliders be dragged beyond the visible limit |
| Text Length | Lunghezza Testo | ✅ | Allows for unlimited text length in text inputs |
| Treasure Room | Stanza del Tesoro | ⚠️ | Sblocca tramite le variabili di gioco (UGV) e le statistiche |
| Unlock Icons | Sblocca Icone | ✅ | Unlocks all icons |
| Unlock Shops | Sblocca Negozi | ⚠️ | Mostra statistiche (monete) sufficienti ad aprire i negozi; gli orbs non vengono toccati |
| Unlock Vaults | Sblocca Vault | ⚠️ | Sblocca tramite le variabili di gioco (UGV) e le statistiche |

## Cosmetic

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Accurate Percentage | Percentuale Accurata | ✅ | Allows decimal place customisation for the in-game Percentage Decimals setting |
| Ball Rotation Bug | Bug Rotazione Ball | ⚠️ | Ricostruzione approssimativa: la rotazione della palla viene raddoppiata |
| Classic Particles | Particelle Classiche | ⚠️ | Riattiva le particelle 'trailing' di PlayerObject: l'aspetto puo' differire dalla 2.1 |
| Classic Pulse | Pulsazione Classica | ⚠️ | Calcola la pulsazione ad ogni frame dal metering FMOD della musica (invece che a 60 Hz) |
| Classic Wave Trail | Wave Trail Classico | ⚠️ | Approssimato: scia dell'onda solida e senza pulsazione |
| Coin Shower | Pioggia di monete | ✅ | AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA |
| Custom Wave Trail |  | ✅ |  |
| Frozen Animations | Animazioni congelate | ✅ | Disables sprite animations like the Robot and Spider |
| Hide Pause Button | Nascondi Pulsante Pausa | ✅ | Hides the pause button when the in-game cursor is enabled |
| Hide Pause Menu | Nascondi Menu Pausa | ✅ | Hides the pause menu, helpful for taking screenshots of levels |
| Hide Player | Nascondi Giocatore | ✅ | Hides the player |
| Icon Randomiser | Randomizzatore icone | ⚠️ | Cambia le icone a ogni livello (valori salvati in GameManager) |
| No Camera | No Camera | ✅ | Disables camera movement triggers |
| No Camera Zoom | No Camera Zoom | ✅ | Disables camera zoom triggers |
| No Circle Effect | No effetto cerchio | ✅ | Removes circle effect from pads, orbs, and portals |
| No Dash Fire | Nascondi scia dash | ✅ | Hides the dash trail fire effect |
| No Death Effect | No Effetto di Morte | ✅ | No visual effects on death |
| No Do Not Flip | No 'Do Not' Flip | ✅ | Stops flipping attempt & level complete text |
| No End Shake | No scuotimento alla fine dei livelli | ✅ | Removes camera shake from level complete animation |
| No Ghost Trail | No scia fantasma | ✅ | Disables the ghost trail effect on players |
| No Glow | No Glow | ✅ | Disables the glow effect on all objects |
| No Mirror | No Specchio | ✅ | Disables the mirror portal effect |
| No New Best Popup | No popup New Best | ✅ | Disables the new best popup and reduces the extended respawn time |
| No Orb Ring | No Orb Ring | ✅ | Disables the orb rings upon touching |
| No Particles | Niente Particles | ✅ | Disables aspects of the particle system |
| No Particles Classic | Niente Particles (classico) | ✅ | Disables the particle system (mostly) |
| No Portal Circle | No Cerchio Portali | ✅ | Disables the inner-circle when entering a portal |
| No Portal Lightning | No Fulmine Portali | ✅ | Disables portal lightning from size-changing portals |
| No Pulse | No Pulsazione | ✅ | Disables pulsing on objects |
| No Respawn Flash | No Respawn Flash | ✅ | Disables the respawn flash & circle effect |
| No Robot Fire | No effetto fuoco Robot | ✅ | Removes boost fire animation and particles from Robot gamemode |
| No Shaders | No Shader | ✅ | Disables in-level shaders |
| No Shake | No Shake | ✅ | Disables shake triggers |
| No Spider Dash | No dash Spider | ✅ | Disables the Spider Dash effect when teleporting |
| No Swing Fire | No fuoco Swing | ✅ | Removes boost fire animation and particles from Swing gamemode |
| No Trail | No Scia | ✅ | Sets the trail to always off |
| No Trail Behind Wave | No Scia Dietro Wave | ✅ | Disables default player trail behind the wave trail |
| No Wave Pulse | No Pulsazione Wave | ✅ | Disables wave pulsing |
| No Wave Trail | No Scia Wave | ✅ | Disables the hard wave trail, standard trail remains |
| Player 1 on Top | Giocatore 1 in Cima | ✅ | Forces player 1 to be above player 2 |
| Player on Top | Giocatore in Cima | ✅ | Forces the player (and wave trail) to appear above all objects |
| Show Total Attempts | Mostra Attempt Totali | ✅ | Shows the total attempt count in-level instead of the session attempt count |
| Solid Wave Trail | Scia Wave Solida | ✅ | No blending on the wave trail, like the black & white trail |
| StartPos Reset Camera | StartPos Reset Camera | ✅ | Force resets each StartPos's camera |
| Stop Triggers on Death | Ferma Trigger Alla Morte | ✅ | Stops move/rotation triggers on death so you can see what killed you |
| Trail Always On |  | ✅ | Keeps player trail always visible |
| Trail Cutting | Taglio Scia | ⚠️ | Approssimato: la scia viene azzerata periodicamente |
| Wave Pulse Size | Dimensione | ✅ | Changes the wave pulse size multiplier |
| Wave Trail on Death | Scia Wave Dopo Morte | ✅ | Keeps wave trail visible on death |

## Creator

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Accurate Save | Salvataggio Accurato | ❌ | Non disponibile: la precisione e' nel formato di GameObject::getSaveString (serve una patch del binario) |
| Copy Hack | Hack Copia | ✅ | Lets you copy any level, without a password |
| Custom Object Bypass | Bypass Oggetti Personalizzati | ✅ | Removes the object limit for custom objects & lets you save over 50 |
| Default Song Bypass | Bypass Canzone Predefinita | ✅ | Lets you use hidden default songs in the editor |
| Editor Extension | Estensione Editor | ✅ | Increases the editor length by a factor of 128 |
| Free Scroll | Scorrimento Libero | ✅ | Allows scrolling out of the editor |
| Hide UI | Nascondi UI | ✅ | Hides the editor UI while building |
| Level Edit | Modifica Livello | ✅ | Lets you edit any level through the pause menu |
| Multiple Editor Trails | Multiple Scie nell'editor | ❌ | Non disponibile: LevelEditorLayer::clearPlayerPoints e' inline su Windows (nessun indirizzo da agganciare) |
| No C Mark | Nessun Marchio C | ✅ | Removes the (C) mark when uploading copied levels |
| No Playtest Zoom |  | ✅ | Stops the editor layer zooming in when playtesting, zoom triggers will also be disabled |
| Object Trajectory |  | ❌ | Non disponibile: richiede la simulazione dei trigger move/rotate |
| Place Over | Posizionamento Sovrapposto | ✅ | Lets you place the same object over itself in the editor |
| Smooth Editor Trail | Scia Editor Fluida | ❌ | Non disponibile: LevelEditorLayer::addPlayerPoint e' inline su Windows (nessun indirizzo da agganciare) |
| Toolbox Button Bypass | Bypass Pulsante Strumenti | ✅ | Allows for more objects in the editor toolbox |
| Trigger Value Bypass | Bypass Valore Trigger | ✅ | Bypasses trigger control limits |
| Verify Hack | Hack Verifica | ✅ | Lets you upload unverified levels |

## Level

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| 0% Practice Complete 🔴 | Completamento Pratica 0 | ✅ | Completes a level if you beat it in 1 practice attempt |
| All Modes Platformer | Tutte le Modalità Platformer | ⚠️ | In GD 2.2 tutte le modalita' sono gia' disponibili in platformer: nessun effetto |
| Allow Pause Buffering | Consenti Buffering di pausa | ✅ | Disables the pause-buffering anticheat |
| Auto Clicker 🔴 | Auto Clicker | ✅ | Automatically applies inputs at exact intervals |
| Auto Deafen | Silenzia Audio Automaticamente | ⚠️ | Premi la scorciatoia di Discord (Ctrl+Shift + tasto scelto) simulando la tastiera |
| Auto Kill | Auto Morte | ✅ | Kills the player at a certain percentage (classic) or time (platformer) |
| Auto Music Sync | Sincronizzazione Musica Automatica | ⚠️ | Solo livelli lineari con una canzone; usa il tempo del livello come riferimento |
| Auto Pickup Coins 🔴 | Raccolta Automatica Monete | ✅ | Automatically collects user & secret coins! |
| Auto Practice Mode |  | ✅ |  |
| Auto Song Download | Download Automatico Canzone | ✅ | Automatically starts downloading songs when you open the level page |
| Auto StartPos 🔴 | Auto StartPos | ⚠️ | Usa PlayLayer::addLiveStartPosObject; le StartPos create compaiono nello StartPos Switcher |
| Checkpoint Limit Bypass | Bypass Limite Checkpoint | ✅ | Bypasses 50 checkpoint limit |
| Click Between Frames | Click Between Frames | ⚠️ | Click Between Frames e' una mod Geode: non disponibile in questa versione standalone. Usa Click Between Steps. |
| Click Between Steps |  | ✅ | Toggles the Click Between Steps in-game setting |
| Click on Steps |  | ✅ | Toggles the Click on Steps in-game setting |
| Collect Coins In Practice | Raccogli Monete In Pratica | ✅ | Allows for secret coins & user coins to be picked up in practice mode. Made by WEGFan |
| Confirm Exit | Conferma Uscita | ✅ | Requires confirmation before exiting a level |
| Confirm Full Reset | Conferma Reset Totale | ✅ | Requires confirmation before fully resetting a level |
| Confirm Normal | Conferma Normale | ✅ | Requires confirmation before exiting practice mode |
| Confirm Practice | Conferma Pratica | ✅ | Requires confirmation before entering practice mode |
| Confirm Reset | Conferma Reset | ✅ | Requires confirmation before resetting the level |
| Everything Hurts 🔴 |  | ✅ |  |
| Force Ice 🔴 | Forza Ghiaccio | ✅ | Forces all blocks to be ice blocks (or not ice if disabled) |
| Force Platformer 🔴 | Forza Platformer | ✅ | Enables platformer mode on all levels |
| Frame Stepper 🔴 | Passo per Frame | ⚠️ | Il passo indietro non e' disponibile senza salvare lo stato completo del livello |
| Freeze Player 🔴 |  | ✅ |  |
| Hitbox Multiplier 🔴 | Moltiplicatore Hitbox | ⚠️ | Applicato all'avvio del livello (riavvia il livello dopo aver cambiato i valori) |
| Ignore ESC |  | ✅ |  |
| Instant Complete 🔴 | Completamento Istantaneo | ✅ | Teleports the player to the end of a level |
| Jump Hack 🔴 | Hack Salto | ✅ | Unlimited jump height |
| Jumpscare | Jumpscare | ✅ | Boo |
| LEVEL/HACK_INSTA_KILL |  | ✅ |  |
| No Collision 🔴 | No Collisione | ✅ |  |
| Noclip 🔴 | Noclip | ✅ | Makes the player invincible. |
| Noclip Limits | Limiti Noclip | ✅ | Kills the player after certain accuracy thresholds, requires Noclip |
| Pause During Complete | Pausa Completazione | ✅ | Allows pausing during the level-complete animation |
| Practice Bug Fix | Fix Bug modalità Pratica | ✅ | Saves & restores player velocity in practice mode |
| Practice Music | Musica Pratica | ✅ | Unlocks the practice music sync setting |
| Random Seed 🔴 | Seed Casuale | ⚠️ | Imposta il seed del generatore C (rand) ad ogni tentativo; i trigger che usano il generatore interno di GD potrebbero non essere influenzati |
| Replay Last Checkpoint | Ripeti Ultimo Checkpoint | ✅ | Respawn from your last practice mode checkpoint after completing a level |
| Respawn Time | Tempo di Respawn | ✅ | Changes player respawn time |
| Shipcopter 🔴 | ShipCopter | ✅ | Adds Ship controls to the Swing mode, keeps Swing physics. Made by Beat |
| Show Hitboxes 🔴 | Mostra Hitbox | ⚠️ | Usa il disegno delle hitbox integrato in GD 2.2; le opzioni di stile dipendono dal gioco |
| Show Hitboxes on Death | Mostra Hitbox Morte | ✅ | Visualises level hitboxes, only on death |
| Show Hitboxes Trail 🔴 | Mostra Traccia Hitbox | ✅ | Shows a trail of where your player's hitbox went during an attempt, Show Hitboxes required |
| Show Layout | Mostra Layout | ✅ | Removes all decoration and colour from levels |
| Show Trajectory | Mostra Traiettoria | ❌ | Non disponibile: richiede la simulazione della fisica di GD |
| Show Triggers | Mostra Trigger | ✅ | Shows triggers in level |
| Smart StartPos | StartPos Intelligente | ⚠️ | Gli offset di LevelSettingsObject non sono validati dai size-check: verificare in gioco |
| StartPos Switcher | Switcher StartPos | ✅ | Lets you switch between multiple start positions in-level |

## Universal

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Allow Low Volume | Consenti Volume Basso | ✅ | Removes snapping to 0% when setting volume to 3% or below |
| Compact Lists | Liste Compatte | ⚠️ | Usa la vista compatta interna di GD per livelli e commenti; riapri la lista dopo averla attivata |
| Custom Background | Sfondo Custom | ⚠️ | Colora lo sfondo dei menu (sprite di sfondo della scena) |
| Fast Chests | Casse Veloci | ✅ | Speeds up the chest animation |
| Load Audio to Memory | Carica Audio alla Memoria | ✅ | Loads audio into memory instead of streaming it from your HDD/SSD, can improve performance while slightly increasing load times |
| Lock Cursor | Blocca Cursore | ✅ | Locks Cursor to window while playing/editing |
| Main Menu Play | Gioca dal Menu Principale | ✅ | Take control of gameplay in the main menu |
| No Menu Animations |  | ✅ |  |
| No Music Fade Out | No Fade Out Audio | ✅ | Stops songs fading out. Made by Beat |
| No Transition | Nessuna Transizione | ✅ | Shortens scene transition time to 0s |
| Pitch Shifter | Cambia Tono | ✅ | Shifts the pitch of background music |
| Thread Priority | Priorità Thread | ✅ | Affects how much CPU time GD gets, can improve performance |
| Transition Customiser | Personalizza Transizioni | ✅ | Customise the type and duration of scene transitions |
| Transparent Lists | Liste Trasparenti | ✅ | Removes the background from list views |

## Status

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Attempts | Tentativi | ✅ |  |
| Best Run | Run migliore | ✅ |  |
| Cheat Indicator | Indicatore Cheat | ✅ |  |
| Clock | Orologio | ✅ |  |
| CPS Counter | Contatore CPS | ✅ |  |
| Dead |  | ✅ |  |
| FPS Counter | Contatore FPS | ✅ |  |
| Frame Counter | Contatore Frame | ✅ |  |
| Jumps | Salti | ✅ |  |
| Level Time | Tempo sul livello | ✅ |  |
| Message | Messaggio | ✅ |  |
| Noclip Accuracy | Accuratezza Noclip | ✅ |  |
| Noclip Deaths | Morti Noclip | ✅ |  |
| Percentage | Percentuale | ✅ |  |
| Position | Posizione | ✅ |  |
| Replay State | Stato del replay | ✅ |  |
| Session Time | Session time | ✅ |  |
| Testmode | Testmode | ✅ |  |
| Velocity |  | ✅ |  |

## Iconic

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Effects | Effetti | ✅ |  |

## Display

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Borderless Classic | Borderless (classico) | ✅ | Classic implementation of borderless fullscreen (separate to GD's own implementation) |
| Frame Extrapolation | Estrapolazione frame | ❌ | Non disponibile: richiede di interpolare la posizione di tutti gli oggetti tra due tick |
| Fullscreen | Schermo intero | ✅ | Allows the game to fill your main display |
| Lock Delta 🔴 | Lock Delta | ✅ | Locks the frame time (delta) during gameplay for consistency, used mainly for botting |
| Physics TPS 🔴 | TPS Fisici | ✅ | Unlocks the default 240/s physics limit, setting it to the target FPS/TPS (whichever is highest). This does NOT replicate 2.1 physics |
| Real Time | Tempo reale | ⚠️ | Approssimato: il delta bloccato viene moltiplicato quando il gioco resta indietro |
| Unlock FPS | Sblocca FPS | ✅ | Sets the FPS target of the game. Physics will not change unless using physics bypass |
| Vertical Sync | Vertical Sync | ✅ | Syncs FPS with display Hz, reducing screen tearing |

## Speedhack

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Classic Mode 🔴 | Modalità classica | ✅ |  |
| Enabled 🔴 | Attivato | ✅ | Slows down the game globally by a set multiplier |
| Speedhack Audio | Audio Speedhack | ✅ | Matches audio speed to the Speedhack |

## Utility

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| AppData | AppData | ✅ | Opens the game's LocalAppData folder |
| Crash Game |  | ✅ |  |
| Left | Sinistra | ✅ |  |
| Left | Sinistra | ✅ |  |
| P1 Click | Click G1 | ✅ |  |
| P2 Click | Click G2 | ✅ |  |
| Practice Mode | Modalità Practice | ✅ | Toggles practice mode in the current level |
| Resources | Risorse | ✅ | Opens the game's Resources folder |
| Restart Level | Ricomincia livello | ✅ | Restarts the current level |
| Right | Destra | ✅ |  |
| Right | Destra | ✅ |  |
| Settings | Impostazioni | ✅ | Opens the in-game settings menu |
| Toggle DevTools | Apri DevTools | ⚠️ | Richiede Geode DevTools: non disponibile senza Geode |
| Uncomplete Level | De-completa livello | ✅ | Uncompletes the current level and removes stats |

## Replay

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Play 🔴 | Riproduci | ✅ |  |
| Record 🔴 | Registra | ✅ |  |

## Cheat Safety

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Auto Safe Mode | Modalità Sicura Automatica | ✅ | Triggers Safe Mode if cheats have been used in an attempt |
| Disable Cheats | Disattiva Cheat | ✅ |  |
| Safe Mode | Modalità Sicura | ✅ | Disables progress, completion & verification of levels |
| Safe Mode Popup | Popup Modalità Sicura | ✅ | Warns before entering a level if cheats are enabled |

## Interface

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Hide Endscreen Cheats | Nascondi Cheat a fine livello | ⚠️ | Questa versione non aggiunge testo alla schermata di fine livello: nessun effetto |
| Hide Endscreen Extras | Nascondi Extra a fine livello | ⚠️ | Questa versione non aggiunge extra alla schermata di fine livello: nessun effetto |
| Hide Iconic on Pause | Nascondi Icone in pausa | ⚠️ | Iconic qui e' una finestra del menu, non un pulsante in pausa: nessun effetto |
| Hide Menu Snow | Nascondi Neve nel menu | ⚠️ | Nessuna neve nel menu di GD 2.209: nessun effetto |
| Hide RobsVault Shortcut |  | ✅ | Hides the main level RobsVault shortcut. Made by Beat |

## Screenshot

| Funzione | Italiano | Stato | Note |
|---|---|---|---|
| Screenshot | Screenshot | ✅ | Takes a screenshot |

