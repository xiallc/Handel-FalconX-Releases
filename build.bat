@echo off
setlocal

:: Locate the waf file by monorepo or project repo.
set waf=%~dp0..\tools\waf\waf
if not exist "%waf%" set waf=platform\waf\waf

python %waf% %*
exit /b %ERRORLEVEL%
