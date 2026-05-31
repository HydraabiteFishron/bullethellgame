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

g++ main.cpp ^
    %BUILD_FLAGS% ^
    -Iinclude ^
    -Llib ^
    -lSDL3 ^
    -o g.exe