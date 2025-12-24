#include <engine/core/profiler.h>
#include <engine/core/time.h>
#include <engine/core/array.h>
#include <engine/core/log.h>
#include <engine/core/ecs.h>
#include <engine/platform/platform.h>

#include <engine/engine.h>

static void
crude_engine_initialize_ecs_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_allocators_
(
  _In_ crude_engine                                       *engine
);

void
crude_engine_initialize
(
  _In_ crude_engine                                       *engine,
  _In_ crude_engine_creation const                        *creation
)
{
  *engine = CRUDE_COMPOUNT_EMPTY( crude_engine );

  crude_log_initialize( );
  crude_time_service_initialize( );
  crude_platform_service_initialize( );
  crude_engine_initialize_ecs_( engine );
  crude_task_sheduler_initialize( &engine->task_sheduler, CRUDE_ENGINE_TOTAL_THREADS_COUNT );
  crude_engine_initialize_allocators_( engine );
  crude_gfx_asynchronous_loader_manager_intiailize( &engine->asynchronous_loader_manager, &engine->task_sheduler, 1u );
}

void
crude_engine_deinitialize
(
  _In_ crude_engine                                       *engine
)
{
  crude_task_sheduler_deinitialize( &engine->task_sheduler );
  crude_gfx_asynchronous_loader_manager_deintiailize( &engine->asynchronous_loader_manager );
  crude_ecs_deinitalize( engine->world );
  crude_platform_service_deinitialize( );
  crude_log_deinitialize( );
}

bool
crude_engine_update
(
  _In_ crude_engine                                       *engine
)
{
  int64                                                    current_time;
  float32                                                  delta_time;
  bool                                                     should_not_quit;

  CRUDE_PROFILER_ZONE_NAME( "crude_engine_update" );
  
  should_not_quit = true;
  if ( crude_ecs_should_quit( engine->world ) )
  {
    should_not_quit = false;
    goto cleanup;
  }
  
  current_time = crude_time_now();
  delta_time = crude_time_delta_seconds( engine->last_update_time, current_time );
  crude_ecs_progress( engine->world, delta_time );
  engine->last_update_time = current_time;

cleanup:
  CRUDE_PROFILER_ZONE_END;
  return true;
}

void
crude_engine_initialize_ecs_
(
  _In_ crude_engine                                       *engine
)
{
  engine->world = crude_ecs_init();
  engine->running = true;
  engine->last_update_time = crude_time_now();

  ECS_TAG_DEFINE( engine->world, crude_entity_tag );
  
  crude_ecs_set_threads( engine->world, 1 );
}

void
crude_engine_initialize_allocators_
(
  _In_ crude_engine                                       *engine
)
{
}