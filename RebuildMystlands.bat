@echo off

set ROOTDIR=%~dp0
set ROOTDIR=%ROOTDIR:~0,-1%

set PROJECT=IslandGamePrototype
set PROJECT_DIR=%ROOTDIR%
set UPROJECT_PATH=%PROJECT_DIR%\%PROJECT%.uproject

set UE_DIR=E:\Unreal Engine\UE_5.5
set REBUILD_PATH=%UE_DIR%\Engine\Build\BatchFiles\Rebuild.bat

call "%REBUILD_PATH%" %PROJECT%Editor Win64 Development "%UPROJECT_PATH%" -waitmutex -NoHotReload

pause
