#include <flecs.h>

#include <core/profiler.h>
#include <core/time.h>
#include <scene/entity.h>
#include <core/array.h>
#include <core/log.h>

#include <engine.h>

static void
pinned_task_run_loop_
(
  _In_ void                                               *ctx
)
{
  crude_engine *engine = ( crude_engine* )ctx;
  
  while( !enkiGetIsShutdownRequested( engine->task_sheduler ) && engine->running )
  {
    enkiWaitForNewPinnedTasks( engine->task_sheduler );
    enkiRunPinnedTasks( engine->task_sheduler );
  }
}

void
crude_engine_initialize
(
  _In_ crude_engine                                       *engine,
  _In_ int32                                               num_threads
)
{
  engine->world   = ecs_init();
  engine->running = true;
  engine->time    = 0;
  
  ECS_TAG_DEFINE( engine->world, Entity );

  if (num_threads > 1)
  {
    ecs_set_threads( engine->world, num_threads );
  }
  
  crude_log_initialize();
  
  crude_heap_allocator_initialize( &engine->algorithms_allocator, 1024 * 1024 * 1024, "AlgorithmsAllocator" );

  crude_time_service_initialize();

  {
    struct enkiTaskSchedulerConfig                         config;

    engine->task_sheduler = enkiNewTaskScheduler();
    config = enkiGetTaskSchedulerConfig( engine->task_sheduler );
    config.numTaskThreadsToCreate += 1;
    enkiInitTaskSchedulerWithConfig( engine->task_sheduler, config );

    engine->pinned_task_loop = enkiCreatePinnedTask( engine->task_sheduler, pinned_task_run_loop_, config.numTaskThreadsToCreate );
    enkiAddPinnedTaskArgs( engine->task_sheduler, engine->pinned_task_loop, engine );
  }
}

void
crude_engine_deinitialize
(
  _In_ crude_engine *engine
)
{
  enkiWaitforAllAndShutdown( engine->task_sheduler );
  enkiDeletePinnedTask( engine->task_sheduler, engine->pinned_task_loop );
  enkiDeleteTaskScheduler( engine->task_sheduler );
  crude_heap_allocator_deinitialize( &engine->algorithms_allocator );
  crude_log_deinitialize();
}

bool
crude_engine_update
(
  _In_ crude_engine *engine
)
{
  ecs_world_t *world = engine->world;
  
  int64 const current_tick = crude_time_now();
  float32 const delta_time = crude_time_delta_seconds( engine->time, current_tick );
  engine->time = current_tick;

  if ( !ecs_should_quit( world ) )
  {
    ecs_world_info_t const *info = ecs_get_world_info( world );
    ecs_progress( world, delta_time );
    return true;
  }

  ecs_fini( world );
  engine->running = false;
  return false;
}