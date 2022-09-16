@echo off
REM Win32 Game Build Script

set CC=gcc
set CFLAGS=-std=c11 -ggdb3 -w -Wno-unused-variable -Wno-unused-function -DDEBUG_BUILD -DSUPPRESS_STUBS
set CLIBS=-lgdi32 -lopengl32
set GAME_NAME=wanderer.exe

REM did not know you could do that. sweet.
set SRCS= src/win32_main.c ^
		  src/input.c ^
		  src/renderer.c ^
		  src/opengl1_renderer.c ^
		  src/wanderer.c ^
		  src/bmp_loader.c ^
		  src/platform_win32.c ^
		  src/mat4x4.c ^
		  src/vec2.c ^
          	  src/config.c ^
		  src/memory_pool.c ^
		  src/easing_functions.c ^
		  src/wanderer_cutscene.c ^
		  src/wanderer_dialogue.c ^
		  src/wanderer_game_event.c ^
	          src/wanderer_magic.c ^
          src/string.c ^
		  src/wanderer_floating_messages.c ^
		  src/wanderer_tilemap.c ^
		  src/wanderer_gui.c ^
		  src/wanderer_game_ui.c ^
		  src/wanderer_lists.c ^
		  src/wanderer_game_input.c ^
		  src/camera.c ^
		  src/collision.c ^
		  src/wanderer_actor.c 

set BUILD_DIRECTORY=./bin/

%CC% %SRCS% %CFLAGS% %CLIBS% -o %BUILD_DIRECTORY%%GAME_NAME% -Ideps/
