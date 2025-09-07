#!/usr/bin/env sh

CC="gcc"

BASE_SOURCES="
src/base/linear_arena.c
src/base/string8.c
src/base/allocator.c
src/base/image.c
src/base/free_list_arena.c
"
#src/base/thread_context.c

PLATFORM_SOURCES="
src/main.c
src/platform_linux.c
src/asset_manager.c
"

GAME_SOURCES="
src/game/game.c
src/game/renderer/render_batch.c
"

RENDERER_SOURCES="
src/renderer/open_gl/renderer_backend.c
"

FLAGS="
      -std=c99
      -fsanitize=address,undefined
      -Wall
      -Wextra
      -pedantic
      -ggdb
      -Wall
      -Wextra
      -pedantic
      -ggdb
      -Wshadow
      -Wcast-align
      -Wunused
      -Wpedantic
      -Wconversion
      -Wsign-conversion
      -Wsign-compare
      -Wnull-dereference
      -Wdouble-promotion
      -Wformat=2
      -Wduplicated-cond
      -Wduplicated-branches
      -Wlogical-op
      -Werror=return-type
      -Werror=incompatible-pointer-types
      -Werror=int-conversion
      -Werror=implicit-function-declaration
      -Werror=overflow
      -Werror=implicit-int

      -Isrc
      -Ideps



"

rm -r build &&
mkdir -p build/base build/platform build/game build/renderer &&

# Base
${CC} ${BASE_SOURCES} ${FLAGS} -fPIC -c &&
mv *.o build/base &&
ar rcs build/libbase.a build/base/*.o &&

# Game
${CC} ${GAME_SOURCES} ${FLAGS} -Lbuild -lbase -fPIC -shared -o build/libgame.so;

# Renderer
${CC} ${RENDERER_SOURCES} ${FLAGS} -fPIC -c &&
mv *.o build/renderer &&
ar rcs build/librenderer.a build/renderer/*.o &&

## Main
${CC} ${PLATFORM_SOURCES} ${FLAGS} -fPIC -Lbuild -lbase -lrenderer  -Ldeps/ -lstb_image \
      `pkg-config --libs --cflags --static glfw3 glew` -pthread -o build/a.out;
