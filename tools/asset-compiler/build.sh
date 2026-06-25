#!/usr/bin/env sh

CFLAGS="
    -Werror
    -fsanitize=address,undefined
    -std=c99
    -Wall
    -Wextra
    -pedantic
    -ggdb
    -Wall
    -Wextra
    -pedantic
    -Wshadow
    -Wcast-align
    -Wunused
    -Wpedantic
    -Wconversion
    -Wstrict-prototypes
    -Wsign-conversion
    -Wsign-compare
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wcast-align
    -Werror=return-type
    -Werror=incompatible-pointer-types
    -Werror=int-conversion
    -Werror=implicit-function-declaration
    -Werror=overflow
    -Werror=implicit-int
    -Wsign-conversion
    -Werror=missing-braces
    -Wno-error=unused-parameter
    -Wno-error=unused-function
    -Wno-error=unused-variable
    -Wno-error=unused-but-set-variable
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Werror=discarded-qualifiers
"


gcc *.c ../../src/base/*.c ../../src/platform/io_linux.c -I../../src ${CFLAGS} -lm
