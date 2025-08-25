#!/usr/bin/env sh

CC="gcc"
SOURCES="src/main.c src/base/linear_arena.c src/base/string8.c src/platform/linux/file.c";
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

      -Isrc
"

${CC} ${SOURCES} ${FLAGS};
