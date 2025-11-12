#include <core/log.h>
#include <core/assert.h>
#include <core/profiler.h>
#include <scene/scripts_components.h>
#include <scene/scene_components.h>
#include <platform/platform_components.h>
#include <physics/physics_components.h>
#include <enemy_components.h>

#include <enemy_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_enemy_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_enemy_creation_observer_ );

static void
crude_enemy_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_enemy *enemies_per_entity = ecs_field( it, crude_enemy, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_enemy                                           *enemy;

    enemy = &enemies_per_entity[ i ];

    enemy->player_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  }
}

static void
crude_enemy_update_system_
(
  _In_ ecs_iter_t *it
)
{
  crude_enemy *enemies_per_entity = ecs_field( it, crude_enemy, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_enemy                                           *enemy;
    crude_transform                                       *enemy_transform;
    crude_transform const                                 *player_transform;
    XMVECTOR                                               enemy_translation;
    XMVECTOR                                               enemy_to_player;
    XMVECTOR                                               enemy_to_player_normalized;
    crude_entity                                           enemy_node;

    enemy = &enemies_per_entity[ i ];
    enemy_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    enemy->player_node = crude_ecs_lookup_entity( it->world, "main.player" ); // !TODO
    CRUDE_ASSERT( crude_entity_valid( enemy->player_node ) );
    
    enemy_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( enemy_node, crude_transform );
    
    player_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( enemy->player_node, crude_transform );
    
    enemy_translation = XMLoadFloat3( &enemy_transform->translation );
    enemy_to_player = XMVectorSubtract( XMLoadFloat3( &player_transform->translation ), enemy_translation );
    enemy_to_player_normalized = XMVector3Normalize( enemy_to_player );
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_enemy_system )
{
  ECS_MODULE( world, crude_enemy_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_enemy_components );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_enemy_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_enemy ) }
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_enemy_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_enemy ) }
  } );
}