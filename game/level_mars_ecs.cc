#include <engine/external/game_ecs.h>

#include <game/level_mars_ecs.h>

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_level_mars_system );

CRUDE_ECS_SYSTEM_DECLARE( crude_level_mars_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_level_mars_create_observer_ );

static void
crude_level_mars_create_observer_ 
(
  ecs_iter_t *it
)
{
  crude_level_mars_system_context *ctx = CRUDE_CAST( crude_level_mars_system_context*, it->ctx );
  crude_level_mars *level_mars_per_entity = ecs_field( it, crude_level_mars, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_mars                                      *level_mars;
    crude_entity                                           level_mars_node;

    level_mars = &level_mars_per_entity[ i ];
    level_mars_node = crude_entity_from_iterator( it, i );
  }
}

static void
crude_level_mars_update_system_ 
(
  ecs_iter_t *it
)
{
  crude_level_mars_system_context                         *ctx;
  crude_level_mars                                        *level_mars_per_entity;

  ctx = CRUDE_CAST( crude_level_mars_system_context*, it->ctx );
  level_mars_per_entity = ecs_field( it, crude_level_mars, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_level_mars                                      *level_mars;
    crude_entity                                           level_mars_node;
    
    level_mars = &level_mars_per_entity[ i ];
    level_mars_node = crude_entity_from_iterator( it, i );

  }
}

void
crude_level_mars_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_level_mars_system_context                    *ctx
)
{
  crude_game_components_import( world );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_level_mars_update_system_, EcsOnUpdate, ctx, { 
    { .id = ecs_id( crude_level_mars ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_level_mars_create_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_level_mars ) }
  } );
}