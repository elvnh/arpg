#ifndef SHADER_H
#define SHADER_H

#include "base/allocator.h"
#include "base/string8.h"
#include "base/list.h"

typedef struct ShaderIncludeDirective {
    LIST_LINKS(ShaderIncludeDirective);

    String absolute_include_path;
    ssize  directive_source_index;
} ShaderIncludeDirective;

DEFINE_LIST(ShaderIncludeDirective, ShaderIncludeList);

ShaderIncludeList shader_get_dependencies(String path, Allocator allocator);

#endif //SHADER_H
