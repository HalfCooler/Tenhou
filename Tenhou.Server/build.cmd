@echo off
if "%~1"=="" (powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1") else (powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Configuration %1)
exit /b %errorlevel%
