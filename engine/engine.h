#pragma once

#include <TaskScheduler_c.h>

#include <core/memory.h>

typedef struct crude_engine
{
  void                                                    *world;
  bool                                                     running;
  int64                                                    time;
  enkiTaskScheduler                                       *task_sheduler;
  enkiPinnedTask                                          *pinned_task_loop;
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