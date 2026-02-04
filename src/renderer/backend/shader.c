#include "shader.h"

#include "base/allocator.h"
#include "base/list.h"
#include "base/string8.h"

#include "platform/file.h"
#include "platform/path.h"
#include "platform/thread_context.h"

#define INCLUDE_DIRECTIVE_STRING str_lit("#include ")

static ShaderIncludeList get_include_directives_in_file(String path, Allocator allocator)
{
    Allocator scratch = thread_ctx_get_allocator();

    String source = os_read_entire_file_as_string(path, allocator);

    String parent_dir = os_get_parent_path(path, scratch);
    os_change_working_directory(parent_dir);

    ShaderIncludeList list = {0};

    ssize i = 0;
    while (i < source.length) {
        ssize next_include_index = str_find_first_occurence_from_index(source, INCLUDE_DIRECTIVE_STRING, i);

        if (next_include_index != -1) {
            ssize path_begin_index = next_include_index + INCLUDE_DIRECTIVE_STRING.length + 1;
            ASSERT(source.data[path_begin_index - 1] == '"');
            ssize path_end_index = str_find_first_occurence_from_index(source, str_lit("\""), path_begin_index);

            String include_string = str_create_span(source, path_begin_index, path_end_index - path_begin_index);
            String canonical_include_path = os_get_canonical_path(include_string, allocator);

            ShaderIncludeDirective *node = allocate_item(allocator, ShaderIncludeDirective);

            node->absolute_include_path = canonical_include_path;
            node->directive_source_index = next_include_index;

            list_push_back(&list, node);

            i = path_end_index + 1;
        } else {
            break;
        }
    }

    return list;
}

static bool file_was_visited(StringList *list, String str)
{
    for (StringNode *node = list_head(list); node; node = list_next(node)) {
        if (str_equal(node->data, str)) {
            return true;
        }
    }

    return false;
}

static void mark_file_as_visited(StringList *list, String str, Allocator allocator)
{
    StringNode *node = allocate_item(allocator, StringNode);
    node->data = str;
    list_push_back(list, node);
}

static ShaderIncludeList get_shader_dependencies_recursive(String path, StringList *handled_files, Allocator allocator)
{
    Allocator scratch = thread_ctx_get_allocator();

    mark_file_as_visited(handled_files, path, scratch);

    ShaderIncludeList includes = get_include_directives_in_file(path, allocator);
    ShaderIncludeList result = {0};

    for (ShaderIncludeDirective *dir = list_head(&includes); dir; dir = list_next(dir)) {
        if (!file_was_visited(handled_files, dir->absolute_include_path)) {
            mark_file_as_visited(handled_files, dir->absolute_include_path, scratch);

            list_push_back(&result, dir);

            ShaderIncludeList transitive_includes = get_shader_dependencies_recursive(dir->absolute_include_path,
                handled_files, allocator);

            list_concat(&result, &transitive_includes);
        }
    }

    return result;
}

ShaderIncludeList shader_get_dependencies(String path, Allocator allocator)
{
    // Helper functions change working directory, so we'll restore it afterwards
    String cwd = os_get_working_directory(thread_ctx_get_allocator());

    String canonical_path = os_get_canonical_path(path, thread_ctx_get_allocator());
    StringList handled_files = {0};
    ShaderIncludeList result = get_shader_dependencies_recursive(canonical_path, &handled_files, allocator);

    os_change_working_directory(cwd);

    return result;
}
