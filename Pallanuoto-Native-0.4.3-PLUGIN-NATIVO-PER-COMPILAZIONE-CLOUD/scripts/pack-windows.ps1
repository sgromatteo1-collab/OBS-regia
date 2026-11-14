$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$root = (Get-Location).Path
$dll = @('stage','build_x64') | ForEach-Object {
    $folder = Join-Path $root $_
    if (Test-Path $folder) { Get-ChildItem -Path $folder -Recurse -File -Filter 'pallanuoto-native.dll' }
} | Select-Object -First 1
if (-not $dll) { throw 'Compilazione incompleta: manca pallanuoto-native.dll.' }
$bytes = [System.IO.File]::ReadAllBytes($dll.FullName)
if ($bytes.Length -lt 1024 -or $bytes[0] -ne 77 -or $bytes[1] -ne 90) {
    throw 'La DLL compilata non ha un formato PE Windows valido.'
}
$peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
if ($peOffset -lt 0 -or ($peOffset+6) -gt $bytes.Length -or [BitConverter]::ToUInt16($bytes,$peOffset+4) -ne 0x8664) {
    throw 'DLL non compilata per Windows x64.'
}
$out = Join-Path $root 'dist/Pallanuoto-Native-0.4.3-Windows-x64'
$plugin = Join-Path $out 'pallanuoto-native'
New-Item -ItemType Directory -Path (Join-Path $plugin 'bin/64bit') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $plugin 'data/locale') -Force | Out-Null
# Layout OBS 33+/34: DLL in plugin root. Layout OBS 32: bin/64bit.
Copy-Item $dll.FullName (Join-Path $plugin 'pallanuoto-native.dll') -Force
Copy-Item $dll.FullName (Join-Path $plugin 'bin/64bit/pallanuoto-native.dll') -Force
Copy-Item 'data/locale/it-IT.ini' (Join-Path $plugin 'data/locale/it-IT.ini') -Force
Copy-Item 'installers/Installa-Windows.bat' $out -Force
Copy-Item 'installers/Installa-Windows.ps1' $out -Force
New-Item -ItemType Directory -Path 'dist' -Force | Out-Null
$archive = Join-Path $root 'dist/Pallanuoto-Native-0.4.3-Windows-x64.zip'
if (Test-Path $archive) { Remove-Item $archive -Force }
Compress-Archive -Path $out -DestinationPath $archive -CompressionLevel Optimal
if ((Get-Item $archive).Length -lt 1024) { throw 'ZIP di installazione incompleto.' }
Write-Host 'OK: plugin DLL OBS Windows x64 confezionato'
