#include <core/profiler.h>
#include <core/time.h>
#include <core/array.h>
#include <core/log.h>
#include <core/ecs.h>
#include <core/ecs_utils.h>

#include <engine.h>

ECS_TAG_DECLARE( crude_on_input_tag );
ECS_TAG_DECLARE( crude_on_update_tag );
ECS_TAG_DECLARE( crude_on_render_tag );

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
  engine->world   = crude_ecs_init();
  engine->running = true;
  engine->time    = 0;

  crude_log_initialize();

  crude_time_service_initialize();
 
  ECS_TAG_DEFINE( engine->world, crude_entity_tag );
  ECS_TAG_DEFINE( engine->world, crude_on_input_tag );
  ECS_TAG_DEFINE( engine->world, crude_on_update_tag );
  ECS_TAG_DEFINE( engine->world, crude_on_render_tag );

  /* Create custom pipeline */
  engine->pipeline.world = engine->world;
  engine->pipeline.handle = ecs_pipeline_init( engine->world, &( ecs_pipeline_desc_t ){
    .query.terms = {
      { .id = EcsSystem },
      { .id = crude_on_input_tag },
      { .id = crude_on_update_tag },
      { .id = crude_on_render_tag },
    }
  } );
 
  // Configure the world to use the custom pipeline
  ecs_set_pipeline( engine->world, engine->pipeline.handle );

  {
    struct enkiTaskSchedulerConfig                         config;

    engine->task_sheduler = enkiNewTaskScheduler();
    config = enkiGetTaskSchedulerConfig( engine->task_sheduler );
    config.numTaskThreadsToCreate += 1;
    enkiInitTaskSchedulerWithConfig( engine->task_sheduler, config );

    engine->pinned_task_loop = enkiCreatePinnedTask( engine->task_sheduler, pinned_task_run_loop_, config.numTaskThreadsToCreate - 1 );
    enkiAddPinnedTaskArgs( engine->task_sheduler, engine->pinned_task_loop, engine );
  }
  
  crude_heap_allocator_initialize( &engine->allocator, CRUDE_RKILO( 16 ), "main_engine_allocator" );

  {
    crude_gfx_asynchronous_loader_manager_creation creation = { .allocator_container = crude_heap_allocator_pack( &engine->allocator ), .task_sheduler = engine->task_sheduler };
    crude_gfx_asynchronous_loader_manager_intiailize( &engine->asynchronous_loader_manager, &creation );
  }
}

void
crude_engine_deinitialize
(
  _In_ crude_engine                                       *engine
)
{
  crude_gfx_asynchronous_loader_manager_deintiailize( &engine->asynchronous_loader_manager );
  crude_heap_allocator_deinitialize( &engine->allocator );
  enkiWaitforAllAndShutdown( engine->task_sheduler );
  enkiDeletePinnedTask( engine->task_sheduler, engine->pinned_task_loop );
  enkiDeleteTaskScheduler( engine->task_sheduler );
  crude_log_deinitialize();
}

bool
crude_engine_update
(
  _In_ crude_engine                                       *engine
)
{
  ecs_world_t *world = engine->world;
  
  int64 const current_tick = crude_time_now();
  float32 const delta_time = crude_time_delta_seconds( engine->time, current_tick );
  engine->time = current_tick;

  if ( !crude_ecs_should_quit( world ) )
  {
    crude_ecs_progress( world, delta_time );
    return true;
  }

  crude_ecs_fini( world );
  engine->running = false;
  return false;
}