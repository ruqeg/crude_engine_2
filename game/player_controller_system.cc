#include <core/log.h>
#include <core/profiler.h>
#include <scene/scripts_components.h>
#include <scene/scene_components.h>
#include <platform/platform_components.h>
#include <physics/physics_components.h>
#include <physics/physics_system.h>
#include <player_controller_components.h>

#include <player_controller_system.h>

CRUDE_ECS_OBSERVER_DECLARE( crude_player_controller_creation_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_player_controller_update_system_ );

static void
crude_player_controller_creation_observer_
(
  _In_ ecs_iter_t *it
)
{
  crude_player_controller *player_controllers_per_entity = ecs_field( it, crude_player_controller, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_player_controller *player_controller = &player_controllers_per_entity[ i ];
    crude_entity entity = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
  }
}

static void
crude_player_controller_update_system_
(
  _In_ ecs_iter_t *it
)
{
  crude_transform *transforms_per_entity = ecs_field( it, crude_transform, 0 );
  crude_player_controller *player_controllere_per_entity = ecs_field( it, crude_player_controller, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_player_controller                               *player_controller;
    crude_input const                                     *input;
    crude_physics_body_handle                             *physics_body;
    crude_entity                                           node;

    transform = &transforms_per_entity[ i ];
    player_controller = &player_controllere_per_entity[ i ];

    node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( player_controller->entity_input, crude_input );
    
    physics_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_body_handle );
    
    crude_physics_body_add_linear_velocity( physics_body, XMVectorScale( XMVectorSet( 0, -9.8, 0, 1 ), it->delta_time * player_controller->weight ) );
    
    if ( player_controller->input_enabled )
    {
      crude_transform                                     *pivot_node_transform;
      crude_entity                                         pivot_node;
      XMMATRIX                                             node_to_world, pivot_to_world;
      XMVECTOR                                             new_translation, basis_pivot_up, basis_pivot_right, basis_node_right, basis_node_up, basis_node_forward;
      int32                                                moving_forward, moving_right;
      
      pivot_node = crude_ecs_lookup_entity_from_parent( node, "pivot" );
      pivot_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( pivot_node, crude_transform );
      pivot_to_world = crude_transform_node_to_world( pivot_node, pivot_node_transform );
      basis_pivot_right = XMVector3Normalize( pivot_to_world.r[ 0 ] );

      moving_forward = input->keys[ 'w' ].current - input->keys[ 's' ].current;
      moving_right = input->keys[ 'd' ].current - input->keys[ 'a' ].current;

      new_translation = XMLoadFloat3( &transform[ i ].translation );
      node_to_world = crude_transform_node_to_world( node, transform );

      basis_node_right = XMVector3Normalize( node_to_world.r[ 0 ] );
      basis_node_up = XMVector3Normalize( node_to_world.r[ 1 ] );
      basis_node_forward = XMVector3Normalize( node_to_world.r[ 2 ] );

      if ( moving_right )
      {
        new_translation = XMVectorAdd( new_translation, XMVectorScale( basis_node_right, player_controller->moving_speed.x * moving_right * it->delta_time ) );
      }
      if ( moving_forward )
      {
        new_translation = XMVectorAdd( new_translation, XMVectorScale( basis_node_forward, player_controller->moving_speed.y * moving_forward * it->delta_time ) );
      }
      XMStoreFloat3( &transform->translation, new_translation );

      if ( input->mouse.right.current )
      {
        XMVECTOR rotation = XMLoadFloat4( &pivot_node_transform->rotation );
        XMVECTOR camera_up = g_XMIdentityR1;

        rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( basis_pivot_right, -player_controller->rotation_speed * input->mouse.rel.y ) );
        rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( camera_up, -player_controller->rotation_speed * input->mouse.rel.x ) );
        XMStoreFloat4( &pivot_node_transform->rotation, rotation );
      }
    }
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_player_controller_system )
{
  ECS_MODULE( world, crude_player_controller_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_player_controller_components );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_player_controller_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_player_controller ) }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_player_controller_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_player_controller ) }
  } );
}