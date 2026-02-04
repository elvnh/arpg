#include <sys/inotify.h>
#include <sys/poll.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <linux/limits.h>

#include "file_watcher.h"

#define INOTIFY_MAX_BUFFER_LENGTH (sizeof(struct inotify_event) + NAME_MAX + 1)

static String get_assets_directory(LinearArena *arena)
{
    String executable_dir = platform_get_executable_directory(la_allocator(arena), arena);
    String result = str_concat(
	executable_dir,
	str_lit("/../"ASSET_DIRECTORY),
	la_allocator(arena)
    );
    result = str_null_terminate(result, la_allocator(arena));

    return result;
}

static String get_shader_directory(LinearArena *arena)
{
    String assets_dir = get_assets_directory(arena);
    String result = str_concat(assets_dir, str_lit(SHADER_DIRECTORY), la_allocator(arena));
    result = str_null_terminate(result, la_allocator(arena));

    return result;
}

static String get_sprite_directory(LinearArena *arena)
{
    String assets_dir = get_assets_directory(arena);
    String result = str_concat(assets_dir, str_lit(SPRITE_DIRECTORY), la_allocator(arena));

    result = str_null_terminate(result, la_allocator(arena));

    return result;
}

static String get_font_directory(LinearArena *arena)
{
    String assets_dir = get_assets_directory(arena);
    String result = str_concat(assets_dir, str_lit(FONT_DIRECTORY), la_allocator(arena));

    result = str_null_terminate(result, la_allocator(arena));

    return result;
}

void *file_watcher_thread(void *user_data)
{
    AssetWatcherContext *ctx = user_data;
    LinearArena scratch = la_create(ctx->allocator, MB(1)) ;

    s32 fd = inotify_init();
    ASSERT(fd >= 0);

    ALIGNAS_T(struct inotify_event) char event_buffer[INOTIFY_MAX_BUFFER_LENGTH];

    String root_path = get_assets_directory(&scratch);
    String shader_path = get_shader_directory(&scratch);
    String sprite_path = get_sprite_directory(&scratch);
    String font_path = get_font_directory(&scratch);

    s32 root_wd = inotify_add_watch(fd, root_path.data, IN_MODIFY);
    s32 shader_wd = inotify_add_watch(fd, shader_path.data, IN_MODIFY);
    s32 sprite_wd = inotify_add_watch(fd, sprite_path.data, IN_MODIFY);
    s32 font_wd = inotify_add_watch(fd, font_path.data, IN_MODIFY);
    ASSERT(root_wd != -1);
    ASSERT(shader_wd != -1);
    ASSERT(sprite_wd != -1);
    ASSERT(font_wd != -1);

    struct pollfd poll_desc = {
        .fd = fd,
        .events = POLLIN,
        .revents = 0
    };

    s32 timeout = 100;

    while (!atomic_load_s32(&ctx->should_terminate)) {
        s32 poll_result = poll(&poll_desc, 1, timeout);
        ASSERT(poll_result >= 0);

        if (poll_result == 0) {
            continue;
        }

        ssize length = read(fd, event_buffer, INOTIFY_MAX_BUFFER_LENGTH);
        ssize buffer_offset = 0;

        while (buffer_offset < length) {
            struct inotify_event *event = (struct inotify_event *)&event_buffer[buffer_offset];

            ASSERT(event->mask & IN_MODIFY);

            if (event->mask & IN_MODIFY) {
                String parent_path = {0};

                if (event->wd == shader_wd) {
                    parent_path = shader_path;
                } else if (event->wd == sprite_wd) {
                    parent_path = sprite_path;
                } else if (event->wd == font_wd) {
                    parent_path = font_path;
		} else {
                    ASSERT(0);
                }

                String name = { event->name, (ssize)event->len };
                StringNode *modified_file = allocate_item(ctx->allocator, StringNode);

                // TODO: instead store the canonical path
                modified_file->data = str_concat(parent_path, name, ctx->allocator);

		mutex_lock(ctx->lock);
                list_push_back(&ctx->asset_reload_queue, modified_file);
		mutex_release(ctx->lock);
            }

            buffer_offset += (SIZEOF(struct inotify_event) + event->len);
        }
    }

    la_destroy(&scratch);

    return 0;
}

void file_watcher_start(AssetWatcherContext *ctx)
{
    ctx->allocator = default_allocator;

    ctx->lock = mutex_create(ctx->allocator);
    pthread_create(&ctx->thread, 0, file_watcher_thread, ctx); // move to init
}

void file_watcher_stop(AssetWatcherContext *ctx)
{
    atomic_store_s32(&ctx->should_terminate, true);
    pthread_join(ctx->thread, 0);
    mutex_destroy(ctx->lock, ctx->allocator);
}

void file_watcher_reload_modified_assets(AssetWatcherContext *ctx, AssetSystem *asset_mgr,
    LinearArena *scratch)
{
    mutex_lock(ctx->lock);

    for (StringNode *file = list_head(&ctx->asset_reload_queue); file;) {
        StringNode *next = file->next;
        AssetSlot *slot = assets_get_asset_by_path(asset_mgr, file->data, scratch);

        b32 should_pop = true;

        if (slot) {
            b32 reloaded = assets_reload_asset(asset_mgr, slot, scratch);

            if (reloaded) {
                printf("Reloaded asset '%s'.\n",
                    str_null_terminate(file->data, la_allocator(scratch)).data);
            } else {
                // Failed to reload, try again later
                should_pop = false;
            }
        }

        if (should_pop) {
            list_pop_head(&ctx->asset_reload_queue);
            deallocate(ctx->allocator, file->data.data);
            deallocate(ctx->allocator, file);
        }

        file = next;
    }

    mutex_release(ctx->lock);
}
