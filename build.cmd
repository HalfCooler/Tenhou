@echo off
setlocal
chcp 65001
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
set "build_result=%errorlevel%"
exit /b %build_result%
