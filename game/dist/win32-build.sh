#!/bin/bash

#*nix or using MSYS.
echo "Cross Compile Script to Win32 from *nix"
x86_64-w64-mingw32-gcc src/win32_main.c src/input.c src/renderer.c src/opengl1_renderer.c src/wanderer.c src/bmp_loader.c src/platform_win32.c src/vec2.c src/mat4x4.c -std=c11 -g -lgdi32 -lopengl32 -Ideps/ -o ./bin/wanderer.exe 
