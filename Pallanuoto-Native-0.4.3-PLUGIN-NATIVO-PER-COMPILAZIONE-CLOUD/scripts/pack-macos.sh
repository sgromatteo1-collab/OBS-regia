#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
root="$PWD"
plugin=""
for parent in "$root/stage" "$root/build_macos"; do
  if [ -d "$parent" ]; then
    plugin=$(find "$parent" -type d -name 'pallanuoto-native.plugin' -print -quit)
    [ -z "$plugin" ] || break
  fi
done
if [ -z "$plugin" ] || [ ! -f "$plugin/Contents/MacOS/pallanuoto-native" ]; then
  echo 'ERRORE: il build non ha prodotto pallanuoto-native.plugin' >&2
  exit 2
fi
# Non accettare plugin vecchio o una compilazione su singola architettura.
archs=$(lipo -archs "$plugin/Contents/MacOS/pallanuoto-native")
case " $archs " in *' arm64 '*) ;; *) echo 'ERRORE: manca ARM64' >&2; exit 3;; esac
case " $archs " in *' x86_64 '*) ;; *) echo 'ERRORE: manca Intel x86_64' >&2; exit 3;; esac
out="$root/dist/Pallanuoto-Native-0.4.3-Mac-Universal"
mkdir -p "$out"
/usr/bin/ditto "$plugin" "$out/pallanuoto-native.plugin"
cp "$root/installers/Installa-Mac.command" "$out/Installa-Mac.command"
chmod +x "$out/Installa-Mac.command"
# Signature ad hoc, without claiming Apple Developer notarization.
codesign --force --deep --sign - "$out/pallanuoto-native.plugin"
codesign --verify --verbose=2 "$out/pallanuoto-native.plugin"
cd "$root/dist"
/usr/bin/ditto -c -k --keepParent "$(basename "$out")" 'Pallanuoto-Native-0.4.3-Mac-Universal.zip'
test -s 'Pallanuoto-Native-0.4.3-Mac-Universal.zip'
echo 'OK: plugin nativo macOS universal confezionato'
