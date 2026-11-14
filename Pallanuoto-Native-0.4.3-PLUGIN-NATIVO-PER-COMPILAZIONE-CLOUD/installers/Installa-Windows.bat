@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Installa-Windows.ps1"
if errorlevel 1 echo Se compare ACCESSO NEGATO, usa il tasto destro: Esegui come amministratore.
pause
