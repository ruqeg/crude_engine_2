#include <flecs.h>

#include <core/profiler.h>
#include <core/time.h>
#include <scene/entity.h>
#include <core/algorithms.h>
#include <core/log.h>

#include <engine.h>

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
  
  crude_initialize_log();
  
  crude_initialize_heap_allocator( &engine->algorithms_allocator, 1024 * 1024 * 1024, "AlgorithmsAllocator" );
  crude_array_set_allocator( &engine->algorithms_allocator );

  crude_initialize_time_service();
}

void
crude_engine_deinitialize
(
  _In_ crude_engine *engine
)
{
  crude_deinitialize_heap_allocator( &engine->algorithms_allocator );
  crude_deinitialize_log();
}

bool
crude_engine_update
(
  _In_ crude_engine *engine
)
{
  ecs_world_t *world = engine->world;
  
  int64 const current_tick = crude_time_now();
  float32 const delta_time = CAST( float32, crude_time_delta_seconds( engine->time, current_tick ) );
  engine->time = current_tick;

  if ( !ecs_should_quit( world ) )
  {
    ecs_world_info_t const *info = ecs_get_world_info( world );
    ecs_progress( world, delta_time );
    CRUDE_PROFILER_MARK_FRAME;
    return true;
  }

  ecs_fini( world );
  engine->running = false;
  return false;
}