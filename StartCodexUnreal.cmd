@echo off
setlocal
set "TERM=xterm-256color"
set "COLORTERM=truecolor"
set "CODEX_UNREAL_EXE="

rem Locate the desktop app's CLI without relying on the Editor's inherited PATH.
for /f "delims=" %%D in ('dir /b /ad /o-d "%LOCALAPPDATA%\OpenAI\Codex\bin" 2^>nul') do if not defined CODEX_UNREAL_EXE if exist "%LOCALAPPDATA%\OpenAI\Codex\bin\%%D\codex.exe" set "CODEX_UNREAL_EXE=%LOCALAPPDATA%\OpenAI\Codex\bin\%%D\codex.exe"
if not defined CODEX_UNREAL_EXE for /f "delims=" %%F in ('where codex.exe 2^>nul') do if not defined CODEX_UNREAL_EXE set "CODEX_UNREAL_EXE=%%F"
if not defined CODEX_UNREAL_EXE (
    echo Codex CLI was not found. Install Codex or add codex.exe to PATH.
    exit /b 1
)

cd /d "%~dp0"
if "%~1"=="" (
    echo Starting Codex with the project's Unreal MCP connection.
    "%CODEX_UNREAL_EXE%" --no-daemon --no-alt-screen -C "%~dp0."
) else (
    "%CODEX_UNREAL_EXE%" --no-daemon --no-alt-screen -C "%~dp0." %*
)
exit /b %ERRORLEVEL%
