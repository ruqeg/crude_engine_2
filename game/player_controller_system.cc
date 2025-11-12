#include <core/log.h>
#include <core/profiler.h>
#include <scene/scripts_components.h>
#include <scene/scene_components.h>
#include <platform/platform_components.h>
#include <physics/physics_components.h>
#include <physics/physics.h>
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
    crude_physics_dynamic_body_handle                     *physics_body;
    crude_entity                                           node;
    XMVECTOR                                               velocity;

    transform = &transforms_per_entity[ i ];
    player_controller = &player_controllere_per_entity[ i ];

    node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( player_controller->entity_input, crude_input );
    
    physics_body = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_dynamic_body_handle );
    
    velocity = crude_physics_dynamic_body_get_velocity( crude_physics_instance( ), *physics_body );
    velocity = XMVectorAdd( velocity, XMVectorScale( XMVectorSet( 0, -9.8, 0, 1 ), it->delta_time * player_controller->weight ) );

    if ( player_controller->input_enabled )
    {
      crude_transform                                     *pivot_node_transform;
      crude_entity                                         pivot_node;
      XMMATRIX                                             pivot_to_world;
      XMVECTOR                                             direction, input_dir, basis_pivot_up, basis_pivot_right, basis_node_right, basis_node_up, basis_node_forward;
      
      pivot_node = crude_ecs_lookup_entity_from_parent( node, "pivot" );
      pivot_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( pivot_node, crude_transform );
      pivot_to_world = crude_transform_node_to_world( pivot_node, pivot_node_transform );
      basis_pivot_right = XMVector3Normalize( pivot_to_world.r[ 0 ] );

      input_dir = XMVectorSet( input->keys[ 'd' ].current - input->keys[ 'a' ].current, 0, input->keys[ 'w' ].current - input->keys[ 's' ].current, 0 );
      
	    direction = XMVector3Normalize( XMVector3TransformNormal( input_dir, pivot_to_world ) );
      
      if ( XMVectorGetX( XMVector3Length( direction ) ) > 0.001f )
      {
        velocity = XMVectorSetX( velocity, XMVectorGetX( direction ) * player_controller->moving_speed.x );
        velocity = XMVectorSetZ( velocity, XMVectorGetZ( direction ) * player_controller->moving_speed.y );
      }
      else
      {
        velocity = XMVectorSetX( velocity, CRUDE_LERP( XMVectorGetX( velocity ), 0.f, CRUDE_MIN( player_controller->moving_speed.x * it->delta_time, 1.f ) ) );
        velocity = XMVectorSetZ( velocity, CRUDE_LERP( XMVectorGetZ( velocity ), 0.f, CRUDE_MIN( player_controller->moving_speed.y * it->delta_time, 1.f ) ) );
      }

      if ( input->mouse.right.current )
      {
        XMVECTOR rotation = XMLoadFloat4( &pivot_node_transform->rotation );
        XMVECTOR camera_up = g_XMIdentityR1;

        rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( basis_pivot_right, -player_controller->rotation_speed * input->mouse.rel.y ) );
        rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( camera_up, -player_controller->rotation_speed * input->mouse.rel.x ) );
        XMStoreFloat4( &pivot_node_transform->rotation, rotation );
      }
    }

    crude_physics_dynamic_body_set_velocity( crude_physics_instance( ), *physics_body, velocity );
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