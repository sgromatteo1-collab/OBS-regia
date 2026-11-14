# Pallanuoto Native 0.4.3

Modulo OBS in C++/Qt: pannello nativo, regia, due formazioni complete, server HTTP/SSE incorporato su 127.0.0.1:18789 e grafica Browser OBS trasparente. Nessun runtime Node. La cartella superiore del pacchetto contiene binari Mac universale e Windows x64, installer e istruzioni utente; VERIFICHE.md distingue i test eseguiti da quelli ancora necessari.

## Build standard Windows
Visual Studio 2022 con Desktop development with C++, Windows SDK 10.0.22621, Git e CMake 3.28+. I download OBS/Qt sono gestiti dal template ufficiale OBS e verificati tramite hash in buildspec.json.

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --parallel 2
cmake --install build_x64 --config RelWithDebInfo --prefix stage
```

## Build standard Mac
Xcode configurato e relativa licenza accettata dall'utente, Git e CMake 3.28+.

```sh
cmake --preset macos
cmake --build --preset macos --parallel 2
cmake --install build_macos --config RelWithDebInfo --prefix stage
```

Il workflow .github/workflows/build-native.yml è facoltativo. Non è stato eseguito per i binari qui distribuiti.

## Build effettivamente utilizzata
Mac: Apple Clang dei Command Line Tools, header ufficiali OBS 31.1.1, framework Qt 6.8.3 da obs-deps 2025-07-11, due architetture e risoluzione dei simboli OBS via dynamic lookup del processo host.

Windows: Zig 0.17.0 / Clang con target MSVC, Microsoft CRT 14.44.17.14 e Windows SDK 10.0.26100 reperiti tramite cargo-xwin dopo accettazione della licenza SDK da parte dell'utente. Qt Windows x64 ufficiale obs-deps 2025-07-11. Linker con startup _DllMainCRTStartup e runtime MSVC dinamico. La cartella cross-windows contiene gli import definition file e uno shim Clang per un intrinsic atteso dagli header OBS 31.1.1. Lo shim non viene usato dal codice regia; consente di compilare gli inline degli header ufficiali.

SDK e librerie proprietarie di sviluppo non sono redistribuiti. Per replicare la cross-compilazione occorrono copie legittime degli SDK e l'accettazione delle relative licenze. Vedi cross-windows/BUILD.md.

## Test
`tests/test.cpp` è un harness Qt con stub per il frontend OBS: verifica validazione, pannello, HTTP/SSE e persistenza senza richiedere OBS aperto. Va compilato separatamente con gli include ufficiali OBS, Qt Core/Gui/Widgets/Network e la risorsa generata da data/assets.qrc. `tests/load.c` verifica dlopen/esportazioni usando le librerie OBS Mac ufficiali.

## Licenza e provenienza
GPL-2.0-or-later, vedi LICENSE. Template di build: https://github.com/obsproject/obs-plugintemplate . Header/SDK: https://github.com/obsproject/obs-studio/tree/31.1.1 . Qt precompilato OBS: https://github.com/obsproject/obs-deps . Nessun codice o binario Fly Scoreboard incluso.

## Risultato live e stile 0.4.0
I segnali valueChanged dei gol e textEdited del periodo salvano e trasmettono solo il relativo campo. Il guard filling blocca pubblicazioni durante caricamento/importazione. Il risultato è un box a due righe in alto a sinistra, con tempo di gioco nel senso di PERIODO: nessun cronometro. Il campo eventLogo è incorporato nella partita e mostrato in presentazione. normalise converte i salvataggi precedenti, elimina i campi dismessi e porta la scena lower a hidden. Il lower third è rimosso dal pannello e dal renderer. I test coprono migrazione, validazione/salvataggio del logo evento e assenza dei vecchi controlli.

## Aggiornamento 0.4.3 — personalizzazione formati e capitano

- Nel pannello OBS, nuova scheda **Grafica e font** con tre profili (Formazioni, Risultato, Presentazione): font del sistema, colore, altezza 10–60 px, grassetto e corsivo. Il profilo scelto viene trasmesso con **Pubblica dati** e salvato in partita.json.
- Nelle schede Casa/Ospiti la terza colonna è una **casella Capitano (C)**, non un campo ruoli. È selezionabile un solo capitano per squadra. Nel video compare esclusivamente la lettera C accanto all'atleta scelto.
- Le calotte numero **1 e 13** (anche scritte 01/013) sono sempre rosse in entrambe le formazioni, sia nel pannello sia nel browser overlay, indipendentemente dai colori scelti.
- Migrazione delle partite 0.4.2: riconosce vecchi ruoli «Capitano» e «C», elimina il campo ruolo dal roster e conserva gli altri dati.
- URL OBS nuovo: `http://127.0.0.1:18789/overlay-v043.html` (rimane disponibile il vecchio URL v042 per compatibilità).
- Lo ZIP 0.4.3 qui fornito contiene **sorgenti aggiornati**, non un modulo .plugin/.dll compilato: per usare queste modifiche in OBS è richiesta una build Mac e/o Windows. I binari 0.4.2 presenti nel pacchetto di partenza non sono stati sostituiti con finti binari 0.4.3.
