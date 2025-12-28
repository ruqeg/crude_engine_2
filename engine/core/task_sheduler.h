#pragma once

#include <TaskScheduler_c.h>

#include <engine/core/resource_pool.h>
#include <engine/core/memory.h>
#include <engine/core/alias.h>

typedef enkiPinnedTaskExecute crude_task_sheduler_pinned_task_fn;
typedef enkiTaskExecuteRange crude_task_sheduler_set_task_fn;

typedef struct crude_task_set_handle
{
  void                                                    *data;
} crude_task_set_handle;

typedef struct crude_task_sheduler
{
  enkiTaskScheduler                                       *enki_task_sheduler;
  enkiPinnedTask                                          *enki_pinned_task_loop;
  enkiPinnedTask                                          *enki_pinned_tasks[ CRUDE_ENGINE_TASK_SHEDULER_PINNED_TASK_ACTIVE_THREAD ];
  uint64                                                   enki_pinned_tasks_count;
  bool                                                     running;
} crude_task_sheduler;

CRUDE_API void
crude_task_sheduler_initialize
(
  _In_ crude_task_sheduler                                *sheduler
);

CRUDE_API void
crude_task_sheduler_deinitialize
(
  _In_ crude_task_sheduler                                *sheduler
);

CRUDE_API void
crude_task_sheduler_add_pinned_task
(
  _In_ crude_task_sheduler                                *sheduler,
  _In_ crude_task_sheduler_pinned_task_fn                  pinned_task_fn,
  _In_ void                                               *ctx,
  _In_ uint64                                              threads_count
);

CRUDE_API crude_task_set_handle
crude_task_sheduler_create_task_set
(
  _In_ crude_task_sheduler                                *sheduler,
  _In_ crude_task_sheduler_set_task_fn                     task_set_fn,
  _In_ void                                               *ctx
);

CRUDE_API void
crude_task_sheduler_destroy_task_set
(
  _In_ crude_task_sheduler                                *sheduler,
  _In_ crude_task_set_handle                               handle
);

CRUDE_API void
crude_task_sheduler_wait_task_set
(
  _In_ crude_task_sheduler                                *sheduler,
  _In_ crude_task_set_handle                               handle
);

CRUDE_API void
crude_task_sheduler_start_task_set
(
  _In_ crude_task_sheduler                                *sheduler,
  _In_ crude_task_set_handle                               handle
);