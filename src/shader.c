#include "shader.h"

#include "base/allocator.h"
#include "base/string8.h"
#include "os/file.h"
#include "os/path.h"
#include "os/thread_context.h"

#define INCLUDE_DIRECTIVE_STRING str_literal("#include ")

static ShaderIncludeList shader_get_include_directives_in_file(String path, Allocator allocator)
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
            ssize path_end_index = str_find_first_occurence_from_index(source, str_literal("\""), path_begin_index);

            String include_string = str_create_span(source, path_begin_index, path_end_index - path_begin_index);
            String canonical_include_path = os_get_canonical_path(include_string, allocator);

            ShaderIncludeDirective *node = allocate_item(allocator, ShaderIncludeDirective);

            node->absolute_include_path = canonical_include_path;
            node->directive_source_index = next_include_index;

            if (!list.tail) {
                ASSERT(!list.head);

                list.head = node;
                list.tail = node;
            } else {
                node->prev = list.tail;
                list.tail->next = node;
                list.tail = node;
            }

            i = path_end_index + 1;
        } else {
            break;
        }
    }

    return list;
}

static bool list_contains(StringList *list, String str)
{
    for (StringNode *node = list->head; node; node = node->next) {
        if (str_equal(node->data, str)) {
            return true;
        }
    }

    return false;
}


static void list_push(StringList *list, String str, Allocator allocator)
{
    StringNode *node = allocate_item(allocator, StringNode);
    node->data = str;

    if (list->head) {
        ASSERT(list->tail);

        node->prev = list->tail;
        list->tail->next = node;
    } else {
        list->head = node;
    }

    list->tail = node;
}



static void filter_out_handled_files(ShaderIncludeList *list, StringList *handled_files)
{
   for (ShaderIncludeDirective *dir = list->head; dir; dir = dir->next) {
        if (list_contains(handled_files, dir->absolute_include_path)) {
            if (dir->prev) {
                dir->prev->next = dir->next;
            } else {
                list->head = dir->next;
            }

            if (dir->next) {
                dir->next->prev = dir->prev;
            } else {
                list->tail = dir->prev;
            }
        }
    }
}

// TODO: linked list macros/functions

static ShaderIncludeList get_shader_dependencies_recursive(String path, StringList *handled_files, Allocator allocator)
{
    Allocator scratch = thread_ctx_get_allocator();
    list_push(handled_files, path, scratch);

    ShaderIncludeList includes = shader_get_include_directives_in_file(path, allocator);
    filter_out_handled_files(&includes, handled_files);

    for (ShaderIncludeDirective *dir = includes.head; dir; dir = dir->next) {
        if (!list_contains(handled_files, dir->absolute_include_path)) {
            ShaderIncludeList transitive_includes = get_shader_dependencies_recursive(dir->absolute_include_path,
                handled_files, allocator);

            if (transitive_includes.head) {
                // Insert the entire list after this node
                transitive_includes.head->prev = dir;
                dir->next = transitive_includes.head;
                includes.tail = transitive_includes.tail;
            }
        }
    }

    return includes;
}

ShaderIncludeList shader_get_dependencies(String path, Allocator allocator)
{
    // Helper functions change working directory, so we'll restore it afterwards
    String cwd = os_get_working_directory(thread_ctx_get_allocator());

    StringList handled_files = {0};
    ShaderIncludeList result = get_shader_dependencies_recursive(path, &handled_files, allocator);

    os_change_working_directory(cwd);

    return result;
}
