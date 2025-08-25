#include <stdio.h>
#include <limits.h>

#include "base/typedefs.h"
#include "base/string8.h"
#include "base/linear_arena.h"
#include "base/allocator.h"
#include "base/utils.h"
#include "platform/file.h"

static void TestsArena()
{
    {
        LinearArena arena = LinearArena_Create(1024);

        s32 *ptr1 = LinearArena_AllocItem(&arena, s32);
        s32 *ptr2 = LinearArena_AllocItem(&arena, s32);

        *ptr1 = 123;
        *ptr2 = 456;

        Assert(*ptr1 == 123);
        Assert(IsAligned((s64)ptr1, AlignOf(s32)));
        Assert(IsAligned((s64)ptr2, AlignOf(s32)));

        *ptr1 = 0;

        Assert(*ptr2 == 456);

        LinearArena_Reset(&arena);
        s32 *p = LinearArena_AllocItem(&arena, s32);
        Assert(p == ptr1);


        LinearArena_Destroy(&arena);
    }
}

static void TestsString()
{
    LinearArena arena = LinearArena_Create(Megabytes(1));
    Allocator allocator = LinearArena_Allocator(&arena);

    {

        String a = String_Literal("hello ");
        String b = String_Literal("world");

        Assert(!String_Equal(a, b));

        String c = String_Concat(a, b, allocator);
        Assert(String_Equal(c, String_Literal("hello world")));
        Assert(c.data != a.data);
        Assert(c.data != b.data);

    }

    {
        String lit = String_Literal("abcdef");
        String copy = String_Copy(lit, allocator);

        Assert(String_Equal(lit, copy));
        Assert(lit.data != copy.data);

        String terminated = String_NullTerminate(copy, allocator);
        Assert(terminated.data != copy.data);
        Assert(terminated.data[terminated.size] == '\0');
        Assert(String_Equal(copy, terminated));
    }

    LinearArena_Destroy(&arena);
}

static void TestsUtils()
{
    {
        char str[] = "hello";
        Assert(ArrayCount(str) == 6);
    }

    Assert(IsPow2(2));
    Assert(IsPow2(4));
    Assert(IsPow2(8));
    Assert(IsPow2(16));

    Assert(!IsPow2(0));
    Assert(MultiplicationOverflows_s64(S64_MAX, 2));
    Assert(!MultiplicationOverflows_s64(S64_MAX, 1));
    Assert(!MultiplicationOverflows_s64(S64_MAX, 0));
    Assert(!MultiplicationOverflows_s64(0, 2));

    Assert(AdditionOverflows_ssize(S_SIZE_MAX, 1));
    Assert(AdditionOverflows_ssize(S_SIZE_MAX - 1, 2));
    Assert(!AdditionOverflows_ssize(S_SIZE_MAX, 0));
    Assert(!AdditionOverflows_ssize(0, 1));

    Assert(S_SIZE_MAX == S64_MAX);

    Assert(Align(5, 8) == 8);
    Assert(Align(8, 8) == 8);
    Assert(Align(16, 8) == 16);
    Assert(Align(17, 8) == 24);
    Assert(Align(0, 8) == 0);
    Assert(Align(4, 2) == 4);

    Assert(IsAligned(8, 8));
    Assert(IsAligned(8, 4));

    Assert(!IsAligned(4, 8));
    Assert(!IsAligned(5, 4));
    Assert(IsAligned(5, 1));
}

int main()
{
    TestsArena();
    TestsString();
    TestsUtils();

    LinearArena arena = LinearArena_Create(Megabytes(1));
    ReadFileResult contents = Platform_ReadEntireFile(String_Literal("src/main.c"), LinearArena_Allocator(&arena));

    printf("%s", contents.file_data);

    LinearArena_Destroy(&arena);

    return 0;
}
