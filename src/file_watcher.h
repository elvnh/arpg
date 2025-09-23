#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include "platform.h"
#include "asset_manager.h"

typedef struct {
    Allocator allocator;
    pthread_t thread;
    Mutex lock;
    StringList asset_reload_queue;
    b32 should_terminate;
} AssetWatcherContext;

// TODO: parameterize paths?
void file_watcher_start(AssetWatcherContext *ctx);
void file_watcher_stop(AssetWatcherContext *ctx);
void file_watcher_reload_modified_assets(AssetWatcherContext *ctx, AssetManager *asset_mgr, LinearArena *scratch);

#endif //FILE_WATCHER_H
