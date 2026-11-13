@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0manage-server.ps1" -Action start %*
