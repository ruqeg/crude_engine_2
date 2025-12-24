#include <engine/core/math.h>
#include <engine/core/log.h>
#include <engine/core/assert.h>

#include <engine/core/task_sheduler.h>

static void
crude_task_sheduler_pinned_task_run_loop_
(
  _In_ void                                               *ctx
);

void
crude_task_sheduler_initialize
(
  _In_ crude_task_sheduler                                *sheduler,
  _In_ uint64                                              total_threads_count
)
{
  struct enkiTaskSchedulerConfig                           config;
  
  sheduler->enki_pinned_tasks_count = 0u;

  sheduler->enki_task_sheduler = enkiNewTaskScheduler( );
  config = enkiGetTaskSchedulerConfig( sheduler->enki_task_sheduler );
  
  CRUDE_ASSERTM(CRUDE_CHANNEL_COLLISIONS, total_threads_count < config.numTaskThreadsToCreate, "Hardware available threads count is smaller then requested. Game may work unpredictable! (available %i < requested %i)", config.numTaskThreadsToCreate, total_threads_count )
  config.numTaskThreadsToCreate = CRUDE_MIN( config.numTaskThreadsToCreate, total_threads_count );
  enkiInitTaskSchedulerWithConfig( sheduler->enki_task_sheduler, config );

  sheduler->enki_pinned_task_loop = enkiCreatePinnedTask( sheduler->enki_task_sheduler, crude_task_sheduler_pinned_task_run_loop_, config.numTaskThreadsToCreate - 1 );
  enkiAddPinnedTaskArgs( sheduler->enki_task_sheduler, sheduler->enki_pinned_task_loop, sheduler );
}

void
crude_task_sheduler_deinitialize
(
  _In_ crude_task_sheduler                                *sheduler
)
{
  enkiWaitforAllAndShutdown( sheduler->enki_task_sheduler );
  enkiDeletePinnedTask( sheduler->enki_task_sheduler, sheduler->enki_pinned_task_loop );
  enkiDeleteTaskScheduler( sheduler->enki_task_sheduler );
}

void
crude_task_sheduler_add_pinned_task
(
  _In_ crude_task_sheduler                                *sheduler,
  _In_ crude_task_sheduler_pinned_task_fn                  pinned_task_fn,
  _In_ void                                               *ctx,
  _In_ uint64                                              threads_count
)
{
  enkiPinnedTask                                          *enki_new_pinned_task;
  struct enkiTaskSchedulerConfig                           config;

  config = enkiGetTaskSchedulerConfig( sheduler->enki_task_sheduler );
  enki_new_pinned_task = enkiCreatePinnedTask( sheduler->enki_task_sheduler, pinned_task_fn, threads_count );
  sheduler->enki_pinned_tasks[ sheduler->enki_pinned_tasks_count++ ] = enki_new_pinned_task;
  enkiAddPinnedTaskArgs( sheduler->enki_task_sheduler, enki_new_pinned_task, ctx );
}

static void
crude_task_sheduler_pinned_task_run_loop_
(
  _In_ void                                               *ctx
)
{
  crude_task_sheduler *sheduler = CRUDE_CAST( crude_task_sheduler*, ctx );
  
  while( !enkiGetIsShutdownRequested( sheduler->enki_task_sheduler ) && sheduler->running )
  {
    enkiWaitForNewPinnedTasks( sheduler->enki_task_sheduler );
    enkiRunPinnedTasks( sheduler->enki_task_sheduler );
  }
}