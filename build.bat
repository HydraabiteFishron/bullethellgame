@echo off
setlocal enabledelayedexpansion

if "%1"=="release" (
    set "BUILD_FLAGS=-O3 -static-libgcc -static-libstdc++ -Wl,--start-group -lwinpthread -Wl,--end-group"
    echo building with release build flags:
    echo %BUILD_FLAGS%
) else (
    echo building dev
    set "BUILD_FLAGS="
)

g++ src\main.cpp src\rendering.cpp ^
    %BUILD_FLAGS% ^
    -Iinclude ^
    -Llib ^
    -lSDL3 -lSDL3_image ^
    -o build\g.exe
