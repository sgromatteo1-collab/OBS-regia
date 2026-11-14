$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$src = Join-Path $root 'pallanuoto-native'
$dll = Join-Path $src 'pallanuoto-native.dll'
if (-not (Test-Path $dll)) { throw 'Manca pallanuoto-native.dll compilata. Usa lo ZIP prodotto da GitHub Actions.' }
if (Get-Process obs64 -ErrorAction SilentlyContinue) { throw 'Chiudi OBS prima di installare il plugin.' }
$pluginRoot = Join-Path $env:ProgramData 'obs-studio\plugins'
$dest = Join-Path $pluginRoot 'pallanuoto-native'
$backupRoot = Join-Path $env:ProgramData 'obs-studio\pallanuoto-plugin-backup'
$backup = Join-Path $backupRoot ('pallanuoto-native-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$temp = Join-Path $pluginRoot ('pallanuoto-native-install-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $pluginRoot -Force | Out-Null
Copy-Item $src $temp -Recurse -Force
$moved = $false
try {
    if (Test-Path $dest) {
        New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
        Move-Item $dest $backup
        $moved = $true
        Write-Host "Backup versione precedente: $backup"
    }
    Move-Item $temp $dest
} catch {
    if (Test-Path $temp) { Remove-Item $temp -Recurse -Force }
    if ($moved -and (Test-Path $backup) -and -not (Test-Path $dest)) { Move-Item $backup $dest }
    throw
}
Write-Host 'Plugin OBS Pallanuoto Native 0.4.3 installato.'
Write-Host 'Apri OBS -> Pannelli -> Pallanuoto - Regia.'
