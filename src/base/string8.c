#include <string.h>

#include "string8.h"
#include "utils.h"

String String_Concat(String a, String b, Allocator alloc)
{
    const ssize total_size = a.length + b.length;
    const usize a_length = Cast_s64_usize(a.length);
    const usize b_length = Cast_s64_usize(b.length);

    char *new_string = 0;
    if (TryExtendAllocation(alloc, a.data, a.length, total_size)) {
        new_string = a.data;
    } else {
        new_string = AllocArray(alloc, char, total_size);

        memcpy(new_string, a.data, a_length);
    }

    memcpy(new_string + a_length, b.data,  b_length);

    String result = {
        .data = new_string,
        .length = total_size
    };

    return result;
}

bool String_Equal(String a, String b)
{
    if (a.length != b.length) {
        return false;
    }

    return (memcmp(a.data, b.data, Cast_s64_usize(a.length)) == 0);
}

String String_NullTerminate(String str, Allocator alloc)
{
    if (AdditionOverflows_ssize(str.length, 1)) {
        Assert(false);
        return NullString;
    }

    char *new_string = 0;

    if (TryExtendAllocation(alloc, str.data, str.length, str.length + 1)) {
        new_string = str.data;
    } else {
        new_string = AllocArray(alloc, char, str.length + 1);
        memcpy(new_string, str.data, Cast_s64_usize(str.length));
    }

    new_string[str.length] = '\0';

    String result = {
        .data = new_string,
        .length = str.length
    };

    return result;
}

String String_Copy(String str, Allocator alloc)
{
    char *new_string = AllocArray(alloc, char, str.length);
    memcpy(new_string, str.data, Cast_s64_usize(str.length));

    String result = {
        .data = new_string,
        .length = str.length
    };

    return result;
}
