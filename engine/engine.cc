#include <core/profiler.h>
#include <core/time.h>
#include <core/array.h>
#include <core/log.h>
#include <core/ecs.h>
#include <platform/platform.h>

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
  _In_ crude_engine_creation const                        *creation
)
{
  struct enkiTaskSchedulerConfig                           config;
  crude_gfx_asynchronous_loader_manager_creation           alm_creation;

  crude_log_initialize( );
  crude_time_service_initialize( );
  crude_platform_service_initialize( );

  *engine = CRUDE_COMPOUNT_EMPTY( crude_engine );
  engine->world = crude_ecs_init();
  engine->running = true;
  engine->last_update_time = crude_time_now();

  ECS_TAG_DEFINE( engine->world, crude_entity_tag );

  engine->task_sheduler = enkiNewTaskScheduler();
  config = enkiGetTaskSchedulerConfig( engine->task_sheduler );
  config.numTaskThreadsToCreate += 1;
  enkiInitTaskSchedulerWithConfig( engine->task_sheduler, config );

  engine->pinned_task_loop = enkiCreatePinnedTask( engine->task_sheduler, pinned_task_run_loop_, config.numTaskThreadsToCreate - 1 );
  enkiAddPinnedTaskArgs( engine->task_sheduler, engine->pinned_task_loop, engine );
  
  crude_heap_allocator_initialize( &engine->asynchronous_loader_manager_allocator, CRUDE_RKILO( 16 ), "asynchronous_loader_manager_allocator" );

  alm_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_asynchronous_loader_manager_creation );
  alm_creation.task_sheduler = engine->task_sheduler;
  alm_creation.allocator_container = crude_heap_allocator_pack( &engine->asynchronous_loader_manager_allocator );
  crude_gfx_asynchronous_loader_manager_intiailize( &engine->asynchronous_loader_manager, &alm_creation );

  engine->resources_path = "\\..\\..\\resources\\";
  engine->shaders_path = "\\..\\..\\shaders\\";
}

void
crude_engine_deinitialize
(
  _In_ crude_engine                                       *engine
)
{
  if ( engine->task_sheduler )
  {
    crude_gfx_asynchronous_loader_manager_deintiailize( &engine->asynchronous_loader_manager );
    crude_heap_allocator_deinitialize( &engine->asynchronous_loader_manager_allocator );
    enkiWaitforAllAndShutdown( engine->task_sheduler );
    enkiDeletePinnedTask( engine->task_sheduler, engine->pinned_task_loop );
    enkiDeleteTaskScheduler( engine->task_sheduler );
  }

  crude_platform_service_deinitialize( );
  crude_log_deinitialize( );
}

bool
crude_engine_update
(
  _In_ crude_engine                                       *engine
)
{
  ecs_world_t                                             *world;
  int64                                                    current_time;
  float32                                                  delta_time;

  world = engine->world;
  if ( crude_ecs_should_quit( world ) )
  {
    crude_ecs_fini( world );
    engine->running = false;
    return false;
  }
  
  current_time = crude_time_now();
  delta_time = crude_time_delta_seconds( engine->last_update_time, current_time );
  crude_ecs_progress( world, delta_time );
  engine->last_update_time = current_time;

  return true;
}