#!/usr/bin/env sh

CC="gcc"
HOT_RELOAD=1

BASE_SOURCES="
src/base/linear_arena.c
src/base/string8.c
src/base/allocator.c
src/base/image.c
src/base/random.c
src/base/free_list_arena.c
"

# TODO: remove hot reload source files if hot reloading isn't enabled
PLATFORM_SOURCES="
src/main.c
src/file_watcher.c
src/font.c
src/hot_reload.c
src/renderer_dispatch.c
src/platform_linux.c
src/asset_manager.c
"

GAME_SOURCES="
src/game/game.c
src/game/world.c
src/game/collision.c
src/game/animation.c
src/game/hitsplat.c
src/game/asset_table.c
src/game/quad_tree.c
src/game/damage.c
src/game/entity_system.c
src/game/tilemap.c
src/game/magic.c
src/game/item_manager.c
src/game/equipment.c
src/game/renderer/render_batch.c
src/game/ui/ui_core.c
src/game/ui/ui_builder.c
src/game/components/particle.c
"

RENDERER_SOURCES="
src/renderer/open_gl/renderer_backend.c
"

TEST_SOURCES="
test/src/base/test_linear_arena.c
test/src/base/test_utils.c
test/src/base/test_list.c
test/src/base/test_sl_list.c
"

TEST_RUNNER_SOURCE="
test/src/test_runner.c
"

FLAGS="
      -DHOT_RELOAD=${HOT_RELOAD}

      -fsanitize=address,undefined
      -std=c99
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
      -Isrc/game
      -Ideps
"

if [ "$1" = "clean" ]; then
    rm -rf build *.o *.so *.a *.out;
else
    if [ ! -f libstb_image.a ]; then
        echo "Compiling stb_image...";
        ${CC} deps/stb_image.c -O3 -lm -c -o stb_image.o && ar rcs libstb_image.a stb_image.o &&
            rm stb_image.o;
    fi

    if [ ! -f libstb_truetype.a ]; then
        echo "Compiling stb_truetype...";
        ${CC} deps/stb_truetype.c -O3 -lm -c -o stb_truetype.o && ar rcs libstb_truetype.a stb_truetype.o &&
            rm stb_truetype.o;
    fi

    rm -rf build/** a.out libbase.a libgame.so librenderer.a;
    mkdir -p build/base build/platform build/game build/renderer &&

    touch build/lock &&

    # Base
    ${CC} ${BASE_SOURCES} ${FLAGS} -fPIC -c &&
    mv *.o build/base &&
    ar rcs libbase.a build/base/*.o &&

    # Renderer
    ${CC} ${RENDERER_SOURCES} ${FLAGS} -fPIC -c &&
    mv *.o build/renderer &&
    ar rcs librenderer.a build/renderer/*.o &&

    # Game
    if [ $HOT_RELOAD = 1 ]; then
        ${CC} ${GAME_SOURCES} ${FLAGS} -L. -lbase -fPIC -shared -o libgame.so;
    else
        ${CC} ${GAME_SOURCES} ${FLAGS} -fPIC -c &&
        mv *.o build/game/ &&
        ar rcs libgame.a build/game/*.o;
    fi

    # Main
    if [ $HOT_RELOAD = 1 ]; then
        ${CC} ${PLATFORM_SOURCES} ${FLAGS} -fPIC -L. -lbase -lrenderer -lstb_image -lstb_truetype \
              `pkg-config --libs --cflags --static glfw3 glew` -pthread -o a.out;
    else
        ${CC} ${PLATFORM_SOURCES} ${FLAGS} -fPIC -L. -lbase -lgame -lrenderer -lstb_image -lstb_truetype \
              `pkg-config --libs --cflags --static glfw3 glew` -pthread -o a.out;
    fi

    # Tests
    # TODO: fix linking to game
    python3 test/generate_test_runner.py ${TEST_SOURCES} -o ${TEST_RUNNER_SOURCE} && \
    ${CC} ${TEST_RUNNER_SOURCE} ${FLAGS} -Itest/include/ -L. -lbase -lstb_image -lm -o tests;

    rm build/lock;
fi
