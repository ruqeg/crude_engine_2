#include <flecs.h>

#include <scene/entity.h>

#include <engine.h>

crude_engine
crude_engine_initialize
(
  _In_ int32 num_threads
)
{
  crude_engine engine;
  engine.world   = ecs_init();
  engine.running = true;
  
  ECS_TAG_DEFINE( engine.world, Entity );

  if (num_threads > 1)
  {
    ecs_set_threads( engine.world, num_threads );
  }

  crude_initialize_log();
  
  return engine;
}

void
crude_engine_deinitialize
(
  _In_ crude_engine *engine
)
{
  crude_deinitialize_log();
}

bool
crude_engine_update
(
  _In_ crude_engine *engine
)
{
  ecs_world_t *world = engine->world;
  
  if ( !ecs_should_quit( world ) )
  {
    ecs_world_info_t const *info = ecs_get_world_info( world );
    ecs_progress( world, info->delta_time );
    return true;
  }

  ecs_fini( world );
  engine->running = false;
  
  return false;
}