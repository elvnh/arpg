#ifndef PARSER_H
#define PARSER_H

#include "base/string8.h"
#include "value.h"

Value *parse(String source, LinearArena *arena);

#endif // PARSER_H
