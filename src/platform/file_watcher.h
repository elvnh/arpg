#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include "platform.h"

typedef struct {
    Allocator allocator;
    pthread_t thread;
    Mutex lock;
    StringList asset_reload_queue;
    b32 should_terminate;
} AssetWatcherContext;

void file_watcher_start(AssetWatcherContext *ctx);
void file_watcher_stop(AssetWatcherContext *ctx);
void file_watcher_reload_modified_assets(AssetWatcherContext *ctx, LinearArena *scratch);

#endif //FILE_WATCHER_H
