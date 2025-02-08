#include <flecs.h>

#include <engine.h>

crude_engine crude_engine_initialize( int32 num_threads )
{
  crude_engine engine;
  engine.world   = ecs_init();
  engine.running = true;
  
  if (num_threads > 1)
  {
    ecs_set_threads( engine.world, num_threads );
  }
  
  return engine;
}

bool crude_engine_update( crude_engine *engine )
{
  ecs_world_t *world = engine->world;
  
  if (!ecs_should_quit(world))
  {
    const ecs_world_info_t *info = ecs_get_world_info( world );
    ecs_progress( world, info->delta_time );
    return true;
  }

  ecs_fini(world);
  engine->running = false;
  
  return false;
}