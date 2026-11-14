#!/bin/zsh
set -e
cd "$(dirname "$0")"
expected="$PWD/pallanuoto-native.plugin/Contents/MacOS/pallanuoto-native"
if [[ ! -f "$expected" ]]; then
  echo 'ERRORE: manca il plugin compilato 0.4.3. Devi estrarre lo ZIP prodotto dal build GitHub Actions.'
  read '?Premi Invio per chiudere'; exit 2
fi
if pgrep -ix obs >/dev/null 2>&1; then
  echo 'Chiudi OBS Studio prima dell’installazione.'
  read '?Premi Invio per chiudere'; exit 3
fi
root="$HOME/Library/Application Support/obs-studio"
dest="$root/plugins/pallanuoto-native.plugin"
backup="$root/pallanuoto-plugin-backup/$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$root/plugins" "$backup"
# Copia in una directory temporanea, quindi sostituisci: in caso di errore non perdi il plugin precedente.
tmp="$root/plugins/.pallanuoto-native.installing.$$"
trap 'rm -rf "$tmp"' EXIT
/usr/bin/ditto "$PWD/pallanuoto-native.plugin" "$tmp"
if [[ -d "$dest" ]]; then
  mv "$dest" "$backup/pallanuoto-native.plugin"
fi
if ! mv "$tmp" "$dest"; then
  if [[ -d "$backup/pallanuoto-native.plugin" ]]; then mv "$backup/pallanuoto-native.plugin" "$dest"; fi
  echo 'ERRORE: impossibile installare, versione precedente ripristinata.'
  read '?Premi Invio per chiudere'; exit 4
fi
trap - EXIT
rmdir "$backup" 2>/dev/null || true
echo 'Installato plugin OBS Pallanuoto Native 0.4.3.'
echo 'Apri OBS -> Pannelli -> Pallanuoto · Regia.'
echo 'Se non presente, controlla Aiuto -> File di log di OBS.'
read '?Premi Invio per chiudere'
