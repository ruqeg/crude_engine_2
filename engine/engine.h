#pragma once

#include <TaskScheduler_c.h>

#include <graphics/asynchronous_loader.h>
#include <core/ecs.h>
#include <core/memory.h>

typedef struct crude_engine
{
  ecs_world_t                                             *world;
  bool                                                     running;
  int64                                                    time;
  enkiTaskScheduler                                       *task_sheduler;
  enkiPinnedTask                                          *pinned_task_loop;
  crude_gfx_asynchronous_loader_manager                    asynchronous_loader_manager;
  crude_heap_allocator                                     allocator;
  char const                                              *resources_path;
} crude_engine;

CRUDE_API void
crude_engine_initialize
(
  _In_ crude_engine                                       *engine,
  _In_ int32                                               num_threads
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