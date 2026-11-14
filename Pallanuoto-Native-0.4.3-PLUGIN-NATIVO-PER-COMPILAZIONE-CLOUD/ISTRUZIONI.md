# Pallanuoto Native 0.4.3 — plugin nativo OBS

**Questo progetto NON è la regia standalone**. È il codice originale C++/Qt del plugin OBS 0.4.2, modificato per la versione 0.4.3. Il pannello continua a comparire in **Pannelli > Pallanuoto · Regia**, con server integrato sulla porta 18789 e grafica OBS all'indirizzo `http://127.0.0.1:18789/overlay-v043.html`.

Modifiche: personalizzazione font, colore, corsivo, grassetto e altezza per formazioni/risultato/presentazione; calotte 1 e 13 rosse per entrambe le squadre; capitano con C senza ruoli.

## Per ottenere le vere versioni installabili (.plugin e .dll)

Non utilizzare il vecchio kit autocompila. Questa cartella contiene i **sorgenti** e il workflow GitHub Actions che fa compilare Mac e Windows **su computer cloud**. Il PC dell'utilizzatore non richiede Xcode o Visual Studio.

1. Su GitHub crea un repository **privato** vuoto, con ramo principale `main` e Actions abilitate.
2. Carica in questo repository **tutti i file e le cartelle qui dentro**, inclusa la cartella nascosta `.github`. I file CMakeLists.txt, src, data, cmake, ecc. devono stare nella radice del repository, non dentro una cartella `Sorgenti`.
3. Apri **Actions > Crea plugin nativo OBS 0.4.3 > Run workflow** (oppure il workflow parte con il push su `main`).
4. Se i due lavori sono verdi, nella stessa esecuzione scarica gli artefatti **Pallanuoto-Native-0.4.3-PLUGIN-Mac** e **Pallanuoto-Native-0.4.3-PLUGIN-Windows**. Ciascuno include uno ZIP interno da estrarre.
5. Mac: estrai e avvia `Installa-Mac.command`. Windows: estrai e avvia `Installa-Windows.bat`, con privilegi amministrativi se richiesti.

**Non sono presenti in questo archivio DLL o plugin 0.4.3 precompilati**. Non riutilizzare la DLL o il bundle 0.4.2: installarli non aggiungerebbe le nuove funzioni. Se il job fallisce, il build non ha prodotto un plugin da installare: conserva i log del job.

## Compatibilità e verifiche

- Sorgenti OBS Studio 31.1.1, dipendenze Qt 2025-07-11 fissate in `buildspec.json` dal progetto originale.
- macOS 12+ Intel / Apple Silicon, Windows x64, previo controllo versione OBS e dipendenze. La verifica reale in OBS è richiesta prima di una diretta.
- Su Windows il pacchetto include sia la disposizione nuova (OBS 33+) sia la struttura precedente (OBS 32), in modo che OBS scelga quella supportata. Per l'installazione portable usare la cartella plugin dell'OBS portable, non ProgramData.
- Gli installer salvano una copia del plugin precedente e preservano i dati delle partite. Non eseguire OBS durante la sostituzione.
- La firma macOS è *ad hoc*, non notarizzata da Apple. Il sistema potrebbe mostrare un avviso di sicurezza.

Questo codice è GPL-2.0-or-later come la base originale.
