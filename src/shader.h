#ifndef SHADER_H
#define SHADER_H

#include "base/allocator.h"
#include "base/string8.h"

typedef struct ShaderIncludeDirective {
    struct ShaderIncludeDirective *next;
    struct ShaderIncludeDirective *prev;

    String relative_include_path;
    ssize  directive_source_index;
} ShaderIncludeDirective;

typedef struct {
    ShaderIncludeDirective *head;
    ShaderIncludeDirective *tail;
} ShaderIncludeList;

ShaderIncludeList shader_get_dependencies(String path, Allocator allocator);

#endif //SHADER_H
