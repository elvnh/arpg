#include <stdio.h>
#include <string.h>
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
        LinearArena arena = LinearArena_Create(DefaultAllocator, 1024);

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

    {
        const s32 size = 1024;
        LinearArena arena = LinearArena_Create(DefaultAllocator, size);

        byte *arr1 = LinearArena_AllocArray(&arena, byte, size / 2);
        byte *arr2 = LinearArena_AllocArray(&arena, byte, size / 2);

        Assert(arr1);
        Assert(arr2);

        LinearArena_Destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = LinearArena_Create(DefaultAllocator, size);

        byte *arr1 = LinearArena_AllocArray(&arena, byte, size / 2);
        byte *arr2 = LinearArena_AllocArray(&arena, byte, size / 2 + 1);

        Assert(arr1);
        Assert(arr2);

        byte *arr3 = LinearArena_AllocArray(&arena, byte, 14);
        Assert(arr3);

        LinearArena_Reset(&arena);
        byte *ptr1 = LinearArena_AllocItem(&arena, byte);
        Assert(ptr1 == arr1);

        byte *ptr2 = LinearArena_AllocArray(&arena, byte, size);
        Assert(ptr2 == arr2);

        LinearArena_Destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = LinearArena_Create(DefaultAllocator, size);

        byte *first = LinearArena_AllocItem(&arena, byte);
        byte *arr1 = LinearArena_Alloc(&arena, 1, 4, 32);

        Assert(first != (arr1 + 1));
        Assert(IsAligned((ssize)arr1, 32));

        byte *arr2 = LinearArena_Alloc(&arena, 1, 4, 256);
        Assert(IsAligned((ssize)arr2, 256));

        LinearArena_Destroy(&arena);
    }

    {
        const s32 size = 1024;
        LinearArena arena = LinearArena_Create(DefaultAllocator, size);

        byte *arr1 = LinearArena_Alloc(&arena, 1, size, 256);
        Assert(IsAligned((ssize)arr1, 256));

        LinearArena_Destroy(&arena);
    }

    {
        // Zero out new allocations
        LinearArena arena = LinearArena_Create(DefaultAllocator, 1024);

        byte *arr1 = LinearArena_AllocArray(&arena, byte, 16);
        memset(arr1, 0xFF, 16);

        LinearArena_Reset(&arena);
        byte *arr2 = LinearArena_AllocArray(&arena, byte, 16);
        Assert(arr2 == arr1);

        for (s32 i = 0; i < 16; ++i) {
            Assert(arr2[i] == 0);
        }

        LinearArena_Destroy(&arena);
    }

    {
        LinearArena arena = LinearArena_Create(DefaultAllocator, 1024);

        byte *ptr = LinearArena_AllocArray(&arena, byte, 16);
        const bool extended = LinearArena_TryExtend(&arena, ptr, 16, 32);
        Assert(extended);

        byte *ptr2 = LinearArena_AllocArray(&arena, byte, 16);
        Assert(ptr2 == (ptr + 32));

        LinearArena_Destroy(&arena);
    }

    {
        LinearArena arena = LinearArena_Create(DefaultAllocator, 1024);

        byte *ptr = LinearArena_AllocArray(&arena, byte, 16);
        byte *ptr2 = LinearArena_AllocArray(&arena, byte, 16);
        bool extended = LinearArena_TryExtend(&arena, ptr, 16, 32);
        Assert(!extended);

        LinearArena_Destroy(&arena);
    }

    {
        LinearArena arena = LinearArena_Create(DefaultAllocator, 1024);

        byte *ptr = LinearArena_AllocArray(&arena, byte, 16);
        bool extended = LinearArena_TryExtend(&arena, ptr, 16, 2000);
        Assert(!extended);

        LinearArena_Destroy(&arena);
    }
}

static void TestsString()
{
    LinearArena arena = LinearArena_Create(DefaultAllocator, Megabytes(1));
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

        LinearArena_AllocItem(&arena, byte); // To prevent from extending in place

        String terminated = String_NullTerminate(copy, allocator);
        Assert(terminated.data != copy.data);
        Assert(terminated.data[terminated.length] == '\0');
        Assert(String_Equal(copy, terminated));
    }

    {
        String lit = String_Literal("abcdef");
        String copy = String_Copy(lit, allocator);

        String terminated = String_NullTerminate(copy, allocator);

        Assert(terminated.data == copy.data);
        Assert(terminated.data[terminated.length] == '\0');
        Assert(String_Equal(copy, terminated));
    }

    {
        // Extend in place
        LinearArena ar = LinearArena_Create(DefaultAllocator, 100);
        Allocator alloc = LinearArena_Allocator(&ar);
        String a = String_Copy(String_Literal("abc"), alloc);
        String b = String_Literal("def");

        String c = String_Concat(a, b, alloc);
        Assert(c.data == a.data);

        LinearArena_Destroy(&ar);
    }

    {
        // Fail to extend in place
        LinearArena ar = LinearArena_Create(DefaultAllocator, 100);
        Allocator alloc = LinearArena_Allocator(&ar);
        String a = String_Copy(String_Literal("abc"), alloc);

        AllocItem(alloc, byte);

        String b = String_Literal("def");

        String c = String_Concat(a, b, alloc);
        Assert(String_Equal(c, String_Literal("abcdef")));

        Assert(c.data != a.data);

        LinearArena_Destroy(&ar);
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
    Assert(MultiplicationOverflows_ssize(S64_MAX, 2));
    Assert(!MultiplicationOverflows_ssize(S64_MAX, 1));
    Assert(!MultiplicationOverflows_ssize(S64_MAX, 0));
    Assert(!MultiplicationOverflows_ssize(0, 2));

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

    Assert(Max(1, 2) == 2);
    Assert(Max(2, 1) == 2);
    Assert(Max(-1, 10) == 10);
}

static void TestsFile()
{
    {
        LinearArena arena = LinearArena_Create(DefaultAllocator, Megabytes(1));
        ssize file_size = Platform_GetFileSize(String_Literal(__FILE__), arena);

        Assert(file_size != -1);

        LinearArena_Destroy(&arena);
    }

    {
        // Check that scratch arena works
        LinearArena arena = LinearArena_Create(DefaultAllocator, 1024);
        LinearArena_AllocArray(&arena, byte, 1022);
        byte *last = LinearArena_AllocArray(&arena, byte, 1);


        ssize file_size = Platform_GetFileSize(String_Literal(__FILE__), arena);
        Assert(file_size != -1);

        byte *next = LinearArena_AllocItem(&arena, byte);
        Assert(next == last + 1);

        LinearArena_Destroy(&arena);
    }

    {
        LinearArena arena = LinearArena_Create(DefaultAllocator, Megabytes(1));
        ReadFileResult contents = Platform_ReadEntireFile(String_Literal(__FILE__), LinearArena_Allocator(&arena));
        Assert(contents.file_data);
        Assert(contents.file_size != -1);

        LinearArena_Destroy(&arena);
    }
}

int main()
{
    TestsArena();
    TestsString();
    TestsUtils();
    TestsFile();

    //printf("%s", contents.file_data);

    return 0;
}
