@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0manage_native_server.ps1" -Action stop %*
