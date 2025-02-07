#include <engine.h>
#include <scene/world.h>
#include <scene/entity.h>

static ecs_query_t *run_on_input;
static ecs_query_t *run_on_begin_render;
static ecs_query_t *run_on_render;
static ecs_query_t *run_on_end_render;

crude_engine crude_engine_initialize( int32 num_threads )
{
  crude_engine engine;
  engine.world   = crude_world_create();
  engine.running = true;

  ECS_TAG_DEFINE( engine.world, crude_entity_tag );
  
  if (num_threads > 1)
  {
    ecs_set_threads( engine.world, num_threads );
  }
  
  ECS_TAG( engine.world, OnInput );
  ECS_TAG( engine.world, OnBeginRender );
  ECS_TAG( engine.world, OnRender );
  ECS_TAG( engine.world, OnEndRender );
  
  //run_on_input        = query_create( engine.world, "OnInput" );
  //run_on_begin_render = query_create( engine.world, "OnBeginRender" );
  //run_on_render       = query_create( engine.world, "OnRender" );
  //run_on_end_render   = query_create( engine.world, "OnEndRender" );
  
  return engine;
}

bool crude_engine_update( crude_engine *engine )
{
  crude_world *world = engine->world;
  
  if (!ecs_should_quit(world))
  {
    ecs_progress(world, 0);
    return true;
  }
}