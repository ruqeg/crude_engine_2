#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/scene_components.h>
#include <engine/platform/platform_components.h>
#include <engine/physics/physics_components.h>
#include <engine/physics/physics.h>
#include <engine/external/game_components.h>
#include <game/game.h>

#include <game/player_controller_system.h>

CRUDE_ECS_OBSERVER_DECLARE( crude_player_controller_creation_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_player_controller_update_system_ );

static void
crude_player_controller_creation_observer_
(
  _In_ ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_player_controller *player_controllers_per_entity = ecs_field( it, crude_player_controller, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_player_controller *player_controller = &player_controllers_per_entity[ i ];
    crude_entity node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    player_controller->fly_mode = false;
    player_controller->input_enabled = true;
    player_controller->fly_speed_scale = 5;
    game->focused_camera_node = crude_ecs_lookup_entity_from_parent( node, "pivot.camera" );
  }
}

static void
crude_player_controller_update_system_
(
  _In_ ecs_iter_t *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_player_controller_update_system_" );
  game_t *game = game_instance( );
  crude_transform *transforms_per_entity = ecs_field( it, crude_transform, 0 );
  crude_player_controller *player_controllere_per_entity = ecs_field( it, crude_player_controller, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_player_controller                               *player_controller;
    crude_input const                                     *input;
    crude_physics_character_body                          *character_body;
    crude_transform                                       *pivot_node_transform;
    crude_physics_character_body_handle                    character_body_handle;
    crude_entity                                           node, pivot_node;
    XMMATRIX                                               pivot_to_world;
    XMVECTOR                                               basis_pivot_right, velocity;
    float32                                                moving_limit;

    transform = &transforms_per_entity[ i ];
    player_controller = &player_controllere_per_entity[ i ];

    node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->platform_node, crude_input );
    
    character_body_handle = *CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_character_body_handle );
    character_body = crude_physics_resources_manager_access_character_body( &game->physics_resources_manager, character_body_handle );
    
    velocity = XMLoadFloat3( &character_body->velocity );
 
    if ( input->keys[ SDL_SCANCODE_LSHIFT ].current )
    {
      moving_limit = player_controller->run_speed;
    }
    else
    {
      moving_limit = player_controller->walk_speed;
    }

    pivot_node = crude_ecs_lookup_entity_from_parent( node, "pivot" );
    pivot_node_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( pivot_node, crude_transform );
    pivot_to_world = crude_transform_node_to_world( pivot_node, pivot_node_transform );
    basis_pivot_right = XMVector3Normalize( pivot_to_world.r[ 0 ] );

    if ( player_controller->fly_mode )
    {
      XMVECTOR                                             input_dir, direction, pivot_node_rotation;

      if ( player_controller->input_enabled )
      {
        input_dir = XMVectorSet( input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current, input->keys[ SDL_SCANCODE_E ].current - input->keys[ SDL_SCANCODE_Q ].current, input->keys[ SDL_SCANCODE_W ].current - input->keys[ SDL_SCANCODE_S ].current, 0 );
        direction = XMVector3Normalize( XMVector3TransformNormal( input_dir, pivot_to_world ) );

        XMStoreFloat3( &transform->translation, XMVectorAdd( XMLoadFloat3( &transform->translation ), XMVectorScale( direction, player_controller->fly_speed_scale * moving_limit * it->delta_time ) ) );

        pivot_node_rotation = XMLoadFloat4( &pivot_node_transform->rotation );
        pivot_node_rotation = XMQuaternionMultiply( pivot_node_rotation, XMQuaternionRotationAxis( basis_pivot_right, -player_controller->rotation_speed * input->mouse.rel.y ) );
        pivot_node_rotation = XMQuaternionMultiply( pivot_node_rotation, XMQuaternionRotationAxis( g_XMIdentityR1, -player_controller->rotation_speed * input->mouse.rel.x ) );
        XMStoreFloat4( &pivot_node_transform->rotation, pivot_node_rotation );
      }
    }
    else
    {
      XMVECTOR                                             input_dir, direction, basis_pivot_up, pivot_node_rotation;
      XMVECTOR                                             basis_node_right, basis_node_up, basis_node_forward;

      if ( character_body->on_floor )
      {
        velocity = XMVectorSetY( velocity, 0.f );
      }
      else
      {
        velocity = XMVectorAdd( velocity, XMVectorScale( XMVectorSet( 0, -9.8, 0, 1 ), it->delta_time * player_controller->weight ) );
      }

      if ( player_controller->input_enabled )
      {
        
        input_dir = XMVectorSet( input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current, 0, input->keys[ SDL_SCANCODE_W ].current - input->keys[ SDL_SCANCODE_S ].current, 0 );
        direction = XMVector3Normalize( XMVector3TransformNormal( input_dir, pivot_to_world ) );

        if ( character_body->on_floor )
        {
          if ( XMVectorGetX( XMVector3Length( direction ) ) > 0.001f )
          {
            velocity = XMVectorSetX( velocity, XMVectorGetX( direction ) * moving_limit );
            velocity = XMVectorSetZ( velocity, XMVectorGetZ( direction ) * moving_limit );
          }
          else
          {
            velocity = XMVectorSetX( velocity, CRUDE_LERP( XMVectorGetX( velocity ), 0.f, CRUDE_MIN( player_controller->stop_change_coeff * it->delta_time, 1.f ) ) );
            velocity = XMVectorSetZ( velocity, CRUDE_LERP( XMVectorGetZ( velocity ), 0.f, CRUDE_MIN( player_controller->stop_change_coeff * it->delta_time, 1.f ) ) );
          }
        }

        {
          XMVECTOR rotation = XMLoadFloat4( &pivot_node_transform->rotation );
          rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( basis_pivot_right, -player_controller->rotation_speed * input->mouse.rel.y ) );
          rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( g_XMIdentityR1, -player_controller->rotation_speed * input->mouse.rel.x ) );
          XMStoreFloat4( &pivot_node_transform->rotation, rotation );
        }
        
        if ( input->keys[ SDL_SCANCODE_SPACE ].current && character_body->on_floor )
        {
          velocity = XMVectorSetY( velocity, player_controller->jump_velocity );
        }
      }
      else
      {
        velocity = XMVectorSetX( velocity, CRUDE_LERP( XMVectorGetX( velocity ), 0.f, CRUDE_MIN( player_controller->stop_change_coeff * it->delta_time, 1.f ) ) );
        velocity = XMVectorSetZ( velocity, CRUDE_LERP( XMVectorGetZ( velocity ), 0.f, CRUDE_MIN( player_controller->stop_change_coeff * it->delta_time, 1.f ) ) );
      }
    }

    XMStoreFloat3( &character_body->velocity, velocity );
  }
  CRUDE_PROFILER_END( "crude_player_controller_update_system_" );
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_player_controller_system )
{
  ECS_MODULE( world, crude_player_controller_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_player_controller_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_player_controller ) }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_player_controller_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_player_controller ) }
  } );
}