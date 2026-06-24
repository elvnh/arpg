#include "parser.h"

#include <stdio.h>

int main(void)
{
    String source = str("foo : {abc : 456}");

    LinearArena arena = la_create(default_allocator, MB(8));
    parse(source, &arena);

    return 0;
}
