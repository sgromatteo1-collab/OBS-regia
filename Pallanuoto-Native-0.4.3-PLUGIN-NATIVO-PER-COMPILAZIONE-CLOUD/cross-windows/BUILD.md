# Cross-compilazione della DLL MSVC da Mac

Questo metodo non richiede GitHub. I binari rilasciati sono stati compilati con Zig 0.17.0, header OBS 31.1.1, Qt ufficiale obs-deps 2025-07-11 e Microsoft CRT/SDK preparati con cargo-xwin. Prima di usare cargo-xwin occorre accettare personalmente la licenza SDK Microsoft indicata dal progetto: https://github.com/rust-cross/cargo-xwin .

Prerequisiti locali (non inclusi): Zig, cargo-xwin, rcc della stessa versione Qt, SDK Microsoft e header OBS. Dopo avere scaricato legittimamente gli SDK, impostare:

```
ZIG=/percorso/zig
ZIG_INCLUDE=/percorso/zig/lib/include
XWIN_ROOT=/percorso/xwin
QT_ROOT=/percorso/qt-windows
OBS_ROOT=/percorso/obs-studio-31.1.1
RCC=/percorso/qt-mac/libexec/rcc
```

Creare una directory build esterna con `obsconfig.h` che definisce `OBS_RELEASE_CANDIDATE` e `OBS_BETA` a 0 e generare `qrc_assets.cpp` con rcc da `data/assets.qrc`. Generare gli import lib con `zig dlltool -m i386:x86-64`, usando obs.def e frontend.def qui presenti.

Compilare plugin.cpp e qrc_assets.cpp in COFF x64 con `zig c++`, usando:

```
-target x86_64-windows-msvc -nostdlib -nostdinc
-fms-extensions -fms-compatibility -fms-compatibility-version=19.44
-D_MT -D_DLL -DWIN32 -DWIN64 -DUNICODE -D_UNICODE -DNDEBUG
-D_CRT_SECURE_NO_WARNINGS -std=c++17 -O2
-include cross-windows/clang-compat.h
```

Include: crt/include; sdk/include/{ucrt,shared,um}; ZIG_INCLUDE; QT_ROOT/include e le sottocartelle QtCore/QtGui/QtWidgets/QtNetwork; OBS_ROOT/libobs; OBS_ROOT/frontend/api; directory del file obsconfig.h.

Linkare con `zig build-lib -dynamic -target x86_64-windows-msvc -fno-compiler-rt -fentry=_DllMainCRTStartup`, oggetti COFF, import lib OBS e Qt6{Core,Gui,Widgets,Network}.lib. Aggiungere le directory xwin crt/lib/x86_64, sdk/lib/ucrt/x86_64 e sdk/lib/um/x86_64, e le librerie msvcrt, msvcprt, vcruntime, ucrt, kernel32, user32, shell32, advapi32, ole32, oleaut32, uuid, ws2_32. Impostare l'output con `-femit-bin=pallanuoto-native.dll`.

Su Mac Zig può avere bisogno di un CC host valido con sysroot esplicito dei Command Line Tools, usato per preparare il linker host. Nel nostro ambiente è stato impostato CC su un piccolo wrapper di Apple Clang con `-isysroot` dell'SDK Mac.

Verificare infine architettura PE32+ x64, flag DLL, entry point CRT, esportazioni OBS e importazioni Qt rispetto alle DLL ufficiali. Questi controlli non sostituiscono un avvio di OBS e una registrazione sul Windows di destinazione. La build standard con Visual Studio descritta nel README rimane un'alternativa.
