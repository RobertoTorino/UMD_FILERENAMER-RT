@echo off
setlocal

set "ROOT=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%build.ps1"
if errorlevel 1 exit /b %errorlevel%

start "" "%ROOT%qt\build"
