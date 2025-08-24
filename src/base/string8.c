#include <string.h>

#include "string8.h"
#include "utils.h"

String String_Concat(String a, String b, Allocator alloc)
{
    const ssize total_size = a.size + b.size;

    char *new_string = AllocArray(alloc, char, total_size);

    const usize a_size = Cast_s64_usize(a.size);
    const usize b_size = Cast_s64_usize(b.size);

    memcpy(new_string, a.data, a_size);
    memcpy(new_string + a_size, b.data,  b_size);

    String result = {
        .data = new_string,
        .size = total_size
    };

    return result;
}

bool String_Equal(String a, String b)
{
    if (a.size != b.size) {
        return false;
    }

    return (memcmp(a.data, b.data, Cast_s64_usize(a.size)) == 0);
}

String String_NullTerminate(String str, Allocator alloc)
{
    char *new_string = AllocArray(alloc, char, str.size + 1);
    new_string[str.size] = '\0';

    String result = {
        .data = new_string,
        .size = str.size
    };

    return result;
}

String String_Copy(String str, Allocator alloc)
{
    char *new_string = AllocArray(alloc, char, str.size);
    memcpy(new_string, str.data, Cast_s64_usize(str.size));

    String result = {
        .data = new_string,
        .size = str.size
    };

    return result;
}
