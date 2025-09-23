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
src/file_watcher.c
src/hot_reload.c
src/renderer_dispatch.c
src/platform_linux.c
src/asset_manager.c
"

GAME_SOURCES="
src/game/game.c
src/game/collision.c
src/game/entity.c
src/game/tilemap.c
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
      -Wcast-align
      -Werror=return-type
      -Werror=incompatible-pointer-types
      -Werror=int-conversion
      -Werror=implicit-function-declaration
      -Werror=overflow
      -Werror=implicit-int
      -Werror=discarded-qualifiers
      -Wsign-conversion
      -Wcast-align

      -Isrc
      -Ideps
"

if [ ! -f libstb_image.a ]; then
    echo "Compiling stb_image...";
    ${CC} deps/stb_image.c -O3 -lm -c -o stb_image.o && ar rcs libstb_image.a stb_image.o &&
        rm stb_image.o;
fi

rm -r build/** a.out libbase.a libgame.so librenderer.a;
mkdir -p build/base build/platform build/game build/renderer &&

touch build/lock &&

# Base
${CC} ${BASE_SOURCES} ${FLAGS} -fPIC -c &&
mv *.o build/base &&
ar rcs libbase.a build/base/*.o &&

# Game
${CC} ${GAME_SOURCES} ${FLAGS} -L. -lbase -fPIC -shared -o libgame.so &&

# Renderer
${CC} ${RENDERER_SOURCES} ${FLAGS} -fPIC -c &&
mv *.o build/renderer &&
ar rcs librenderer.a build/renderer/*.o &&

## Main
${CC} ${PLATFORM_SOURCES} ${FLAGS} -fPIC -L. -lbase -lrenderer -lstb_image \
      `pkg-config --libs --cflags --static glfw3 glew` -pthread -o a.out;

rm build/lock;
