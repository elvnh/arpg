# TODO:
#  - don't make all options global
#  - debug flag so that asserts etc are disabled
#  - move warnings to separate file

cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED 99)

project(arpg VERSION 0.1)

find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)

set(
  BASE_LAYER_SOURCES
  src/base/linear_arena.c
  src/base/string8.c
  src/base/allocator.c
  src/base/image.c
  src/base/random.c
  src/base/free_list_arena.c
)

set(
  PLATFORM_LAYER_SOURCES
  src/main.c
  src/platform/file_watcher.c
  src/platform/font.c
  src/platform/platform_linux.c
  src/platform/asset_system.c
  src/platform/stb.c
)

set(
  GAME_SOURCES
  src/game/game.c
  src/game/world.c
  src/game/trigger.c
  src/game/health.c
  src/game/collision.c
  src/game/collision_event.c
  src/game/collision_policy.c
  src/game/animation.c
  src/game/debug.c
  src/game/hitsplat.c
  src/game/asset_table.c
  src/game/game_ui.c
  src/game/quad_tree.c
  src/game/damage.c
  src/game/entity_system.c
  src/game/tilemap.c
  src/game/ai.c
  src/game/entity_state.c
  src/game/magic.c
  src/game/stats.c
  src/game/light.c
  src/base/polygon.c
  src/game/line_of_sight.c
  src/game/item_system.c
  src/game/equipment.c
  src/game/ui/ui_core.c
  src/game/ui/ui_builder.c
  src/game/status_effect.c
  src/game/components/particle.c
  src/game/components/event_listener.c
)

set(
  RENDERER_FRONTEND_SOURCES
  src/renderer/frontend/render_batch.c
)

set(
  RENDERER_BACKEND_SOURCES
  src/renderer/backend/open_gl/renderer_backend.c
  src/renderer/backend/renderer_dispatch.c
)

set(BASE_LIB_NAME base)
set(GAME_LIB_NAME game)
set(RENDERER_LIB_NAME renderer)

set(COMPILATION_LOCK_FILE ${CMAKE_CURRENT_BINARY_DIR}/lock)

set(HOT_RELOAD 1)

add_compile_options(
  -Werror
  -fsanitize=address,undefined
  -std=c99
  -Wall
  -Wextra
  -pedantic
  -ggdb
  -Wall
  -Wextra
  -pedantic
  -Wshadow
  -Wcast-align
  -Wunused
  -Wpedantic
  -Wconversion
  -Wstrict-prototypes
  -Wsign-conversion
  -Wsign-compare
  -Wnull-dereference
  -Wdouble-promotion
  -Wformat=2
  -Wduplicated-cond
  -Wduplicated-branches
  -Wlogical-op
  -Wcast-align
  -Werror=return-type
  -Werror=incompatible-pointer-types
  -Werror=int-conversion
  -Werror=implicit-function-declaration
  -Werror=overflow
  -Werror=implicit-int
  -Werror=discarded-qualifiers
  -Wsign-conversion
  -Werror=missing-braces
  -Wno-error=unused-parameter
  -Wno-error=unused-function
  -Wno-error=unused-variable
)

add_compile_definitions(
  HOT_RELOAD=${HOT_RELOAD}
)

add_link_options(
  -fsanitize=address,undefined
)

include_directories(
  src
  src/game
)

include_directories(
  SYSTEM
  deps
)

add_library(
    ${RENDERER_LIB_NAME}
    SHARED
    ${RENDERER_BACKEND_SOURCES}
)

if(${HOT_RELOAD} EQUAL 1)

  set(
    PLATFORM_LAYER_SOURCES
    ${PLATFORM_LAYER_SOURCES} src/platform/hot_reload.c
  )

  add_executable(
    ${PROJECT_NAME}
    ${PLATFORM_LAYER_SOURCES}
    ${RENDERER_BACKEND_SOURCES}
  )

  add_library(
    ${BASE_LIB_NAME}
    SHARED
    ${BASE_LAYER_SOURCES}
    )

  add_library(
    ${GAME_LIB_NAME}
    SHARED
    ${GAME_SOURCES}
    ${RENDERER_FRONTEND_SOURCES}
  )

  target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
    -DGAME_SO_PATH="$<TARGET_FILE:${GAME_LIB_NAME}>"
    -DCOMPILATION_LOCK_FILE_PATH="${COMPILATION_LOCK_FILE}"
  )

  target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ${BASE_LIB_NAME}
  )

  # Create lock file during build so we don't try to hot reload before compilation is finished
  add_custom_command(
    TARGET ${GAME_LIB_NAME}
    PRE_BUILD
    COMMAND touch ${COMPILATION_LOCK_FILE}
  )

  add_custom_command(
    TARGET ${GAME_LIB_NAME}
    POST_BUILD
    COMMAND rm ${COMPILATION_LOCK_FILE}
  )

else()

  add_executable(
    ${PROJECT_NAME}
    ${PLATFORM_LAYER_SOURCES}
    ${RENDERER_BACKEND_SOURCES}
  )

  add_library(
    ${BASE_LIB_NAME}
    STATIC
    ${BASE_LAYER_SOURCES}
  )

  add_library(
    ${GAME_LIB_NAME}
    STATIC
    ${GAME_SOURCES}
    ${RENDERER_FRONTEND_SOURCES}
  )

  target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ${BASE_LIB_NAME}
    ${GAME_LIB_NAME}
  )

endif()

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  GL
  GLEW
  glfw
  X11
  m
)
