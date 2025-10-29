#pragma once

#include <TaskScheduler_c.h>

#include <graphics/asynchronous_loader.h>
#include <core/ecs.h>
#include <core/memory.h>

typedef struct crude_engine_creation
{
  char const                                              *resource_path;
  char const                                              *shader_path;
} crude_engine_creation;

typedef struct crude_engine
{
  ecs_world_t                                             *world;
  bool                                                     running;
  enkiTaskScheduler                                       *task_sheduler;
  enkiPinnedTask                                          *pinned_task_loop;
  crude_gfx_asynchronous_loader_manager                    asynchronous_loader_manager;
  crude_heap_allocator                                     asynchronous_loader_manager_allocator;
  char const                                              *resources_path;
  char const                                              *shaders_path;
  int64                                                    last_update_time;
} crude_engine;

CRUDE_API void
crude_engine_initialize
(
  _In_ crude_engine                                       *engine,
  _In_ crude_engine_creation const                        *creation
);

CRUDE_API void
crude_engine_deinitialize
(
  _In_ crude_engine                                       *engine
);

CRUDE_API bool
crude_engine_update
(
  _In_ crude_engine                                       *engine
);