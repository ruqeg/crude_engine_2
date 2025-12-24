#pragma once

#include <engine/graphics/asynchronous_loader_manager.h>
#include <engine/core/task_sheduler.h>
#include <engine/core/ecs.h>
#include <engine/core/memory.h>

typedef struct crude_engine_creation
{
} crude_engine_creation;

typedef struct crude_engine
{
  ecs_world_t                                             *world;
  
  crude_task_sheduler                                      task_sheduler;
  crude_gfx_asynchronous_loader_manager                    asynchronous_loader_manager;
  
  bool                                                     running;
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