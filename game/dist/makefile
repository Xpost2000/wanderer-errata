CC=gcc
# CFLAGS=-std=c11 -O2 -DDEBUG_BUILD -DDEVELOPER -Wno-unused-variable -Wno-unused-function -Wno-missing-braces -Wno-unused-parameter -Wall -Wextra 
CFLAGS=-std=c11 -ggdb3 -DDEBUG_BUILD -DDEVELOPER -w
CLIBS=-lm -lSDL2 -lGL
GAME_NAME=wanderer.out

BUILD_DIRECTORY=./bin/

SRCS=src/sdl_main.c\
	 src/input.c\
	 src/renderer.c\
	 src/opengl1_renderer.c\
	 src/platform_linux.c\
	 src/wanderer.c\
	 src/easing_functions.c\
	 src/bmp_loader.c\
	 src/vec2.c\
src/wanderer_magic.c\
	 src/config.c\
	 src/memory_pool.c\
	 src/wanderer_cutscene.c\
	 src/wanderer_dialogue.c\
	 src/wanderer_game_event.c\
	 src/wanderer_floating_messages.c\
	 src/wanderer_tilemap.c\
	 src/wanderer_gui.c\
	 src/wanderer_game_ui.c\
	 src/wanderer_lists.c\
	 src/wanderer_game_input.c\
	 src/string.c\
	 src/camera.c\
	 src/collision.c\
	 src/wanderer_actor.c\
	 src/mat4x4.c

all: $(GAME_NAME)
$(GAME_NAME): $(SRCS)
	$(CC) $(SRCS) -Ideps/ $(CFLAGS) $(CLIBS) -o $(BUILD_DIRECTORY)/$(GAME_NAME)
