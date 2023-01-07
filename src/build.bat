@echo off
setlocal
SET RAYLIB_PATH=C:\raylib\raylib
SET COMPILER_PATH=C:\raylib\w64devkit\bin
SET PATH=%COMPILER_PATH%
SET CFLAGS=%RAYLIB_PATH%\src\raylib.rc.data -s -static -O2 -std=c99 -Wall -I%RAYLIB_PATH%\src -Iexternal -DPLATFORM_DESKTOP
SET LDFLAGS=-lraylib -lopengl32 -lgdi32 -lwinmm

rem f exist .\export_tiles.exe del /F .\export_tiles.exe
rem gcc -o .\export_tiles.exe export_tiles.c %CFLAGS% %LDFLAGS%
rem export_tiles

if exist ..\bin\pflatn.exe del /F ..\bin\pflatn.exe
gcc -o ..\bin\pflatn.exe pflatn.c %CFLAGS% %LDFLAGS% 2> build.log
endlocal
