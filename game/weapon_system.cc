#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/scene_components.h>
#include <engine/platform/platform_components.h>
#include <engine/physics/physics_components.h>
#include <engine/physics/physics.h>
#include <engine/external/game_components.h>
#include <game/game.h>

#include <game/weapon_system.h>

CRUDE_ECS_OBSERVER_DECLARE( crude_weapon_creation_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_weapon_update_system_ );

static void
crude_weapon_creation_observer_
(
  _In_ ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_weapon *weapon_per_entity = ecs_field( it, crude_weapon, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_weapon *weapon = &weapon_per_entity[ i ];
    crude_entity node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
    weapon->weapon_basic_node  = crude_ecs_lookup_entity_from_parent( node, "weapon_basic_position" );
    weapon->weapon_scoped_node = crude_ecs_lookup_entity_from_parent( node, "weapon_scoped_position" );
    weapon->weapon_model_node = crude_ecs_lookup_entity_from_parent( node, "weapon_model" );
    weapon->last_shot_timer = 0.f;
  }
}

void
crude_weapon_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_weapon_update_system_" );
  game_t *game = game_instance( );

  crude_input *input = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->platform_node, crude_input );
  crude_weapon *weapon_per_entity = ecs_field( it, crude_weapon, 0 );
  crude_transform *transforms_per_entity = ecs_field( it, crude_transform, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_weapon                                          *weapon;
    crude_transform                                       *transform;
    crude_transform                                       *weapon_model_transform;
    
    weapon = &weapon_per_entity[ i ];
    transform = &transforms_per_entity[ i ];

    weapon_model_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( weapon->weapon_model_node, crude_transform );
    if ( input->mouse.right.current )
    {
      crude_transform *weapon_scoped_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( weapon->weapon_scoped_node, crude_transform );
      XMStoreFloat3( &weapon_model_transform->translation, XMVectorLerp( XMLoadFloat3( &weapon_model_transform->translation ), XMLoadFloat3( &weapon_scoped_transform->translation ), CRUDE_GAME_WEAPON_MODEL_MOVE_SPEED * it->delta_time ) ); 
      XMStoreFloat4( &weapon_model_transform->rotation, XMVectorLerp( XMLoadFloat4( &weapon_model_transform->rotation ), XMLoadFloat4( &weapon_scoped_transform->rotation ), CRUDE_GAME_WEAPON_MODEL_MOVE_SPEED * it->delta_time ) ); 
    }
    else
    {
      crude_transform *weapon_basic_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( weapon->weapon_basic_node, crude_transform );
      XMStoreFloat3( &weapon_model_transform->translation, XMVectorLerp( XMLoadFloat3( &weapon_model_transform->translation ), XMLoadFloat3( &weapon_basic_transform->translation ), CRUDE_GAME_WEAPON_MODEL_MOVE_SPEED * it->delta_time ) );
      XMStoreFloat4( &weapon_model_transform->rotation, XMVectorLerp( XMLoadFloat4( &weapon_model_transform->rotation ), XMLoadFloat4( &weapon_basic_transform->rotation ), 2 * it->delta_time ) ); 
    }

    if ( input->mouse.left.current && ( weapon->last_shot_timer > CRUDE_GAME_WEAPON_MODEL_SHOT_INTEVAL ) )
    {
      weapon->last_shot_timer = 0.f;
      XMStoreFloat4( &weapon_model_transform->rotation, XMQuaternionMultiply( XMLoadFloat4( &weapon_model_transform->rotation ), XMQuaternionRotationAxis( XMVectorSet( 1, 0, 0, 0 ), -XM_PIDIV2 ) ) );
    }

    weapon->last_shot_timer += it->delta_time;
  }
  CRUDE_PROFILER_END;
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_weapon_system )
{
  ECS_MODULE( world, crude_weapon_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_weapon_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_weapon ) },
    { .id = ecs_id( crude_transform ) },
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_weapon_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_weapon ) }
  } );
}