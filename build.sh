#!/usr/bin/env sh

CC="gcc"
SOURCES="
src/main.c

src/base/linear_arena.c
src/base/string8.c
src/base/allocator.c
src/base/free_list_arena.c

src/os/linux/file.c
src/os/linux/path.c
src/os/linux/thread_context.c
src/os/linux/window.c

src/render/backend/open_gl/renderer_backend.c
src/render/render_batch.c

src/image.c
src/shader.c
src/assets.c

deps/stb_image.c

";
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
      -pthread

      `pkg-config --libs --cflags --static glfw3 glew`
"

${CC} ${SOURCES} ${FLAGS};
