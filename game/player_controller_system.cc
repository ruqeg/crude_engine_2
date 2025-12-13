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
  crude_player *player_per_entity = ecs_field( it, crude_player, 2 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_player_controller                               *player_controller;
    crude_input const                                     *input;
    crude_player                                          *player;
    crude_physics_character_body                          *character_body;
    crude_transform                                       *pivot_node_transform;
    crude_physics_character_body_handle                    character_body_handle;
    crude_entity                                           node, pivot_node;
    XMMATRIX                                               pivot_to_world;
    XMVECTOR                                               basis_pivot_right, velocity;
    float32                                                moving_limit, rotation_speed;

    transform = &transforms_per_entity[ i ];
    player_controller = &player_controllere_per_entity[ i ];
    player = &player_per_entity[ i ];
    node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->platform_node, crude_input );

    pivot_node = crude_ecs_lookup_entity_from_parent( node, "pivot" );
    
    if ( game->death_screen )
    {
      crude_audio_device_sound_set_volume( &game->audio_device, player->walking_sound_handle, 0.f );
      continue;
    }

    character_body_handle = *CRUDE_ENTITY_GET_MUTABLE_COMPONENT( node, crude_physics_character_body_handle );
    character_body = crude_physics_resources_manager_access_character_body( &game->physics_resources_manager, character_body_handle );
    
    velocity = XMLoadFloat3( &character_body->velocity );

    crude_audio_device_sound_set_volume( &game->audio_device, player->walking_sound_handle, player_controller->walking_sound_volume );
    player_controller->walking_sound_volume = 3.f * CRUDE_MIN( XMVectorGetX( XMVector2Length( XMVectorSet( XMVectorGetX( velocity ), XMVectorGetZ( velocity ), 0.f, 0.f ) ) ) / player_controller->walk_speed, 1.f );

    if ( input->keys[ SDL_SCANCODE_1 ].current || input->keys[ SDL_SCANCODE_2 ].current || input->keys[ SDL_SCANCODE_3 ].current )
    {
      uint32 item_slot = 0u;
      if ( input->keys[ SDL_SCANCODE_1 ].current )
      {
        item_slot = 0;
      }
      else if ( input->keys[ SDL_SCANCODE_2 ].current )
      {
        item_slot = 1;
      }
      else if ( input->keys[ SDL_SCANCODE_3 ].current )
      {
        item_slot = 2;
      }
      if ( player->inventory_items[ item_slot ] == CRUDE_GAME_ITEM_SYRINGE_DRUG || player->inventory_items[ item_slot ] == CRUDE_GAME_ITEM_SYRINGE_HEALTH || player->inventory_items[ item_slot ] == CRUDE_GAME_ITEM_AMMUNITION )
      {
        switch ( player->inventory_items[ item_slot ] )
        {
        case CRUDE_GAME_ITEM_SYRINGE_DRUG:
        {
          crude_audio_device_sound_start( &game->audio_device, player->syringe_sound_handle );
          player->drug_withdrawal = CRUDE_MAX( player->drug_withdrawal - CRUDE_GAME_ITEM_SYRINGE_DRUG_WITHDRAWAL_REMOVE, 0.f );
          break;
        }
        case CRUDE_GAME_ITEM_SYRINGE_HEALTH:
        {
          crude_audio_device_sound_start( &game->audio_device, player->syringe_sound_handle );
          player->health = CRUDE_MIN( 1.f, player->health + CRUDE_GAME_ITEM_SYRINGE_HEALTH_ADD );
          break;
        }
        case CRUDE_GAME_ITEM_AMMUNITION:
        {
          crude_audio_device_sound_start( &game->audio_device, player->reload_sound_handle );
          crude_weapon *weapon = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_ecs_lookup_entity_from_parent( pivot_node, "weapon" ), crude_weapon );
          weapon->ammo = 10;
          break;
        }
        }
        game_player_set_item( game, player, item_slot, CRUDE_GAME_ITEM_NONE );
      }
    }

    if ( input->keys[ SDL_SCANCODE_LSHIFT ].current )
    {
      moving_limit = player_controller->run_speed;
    }
    else
    {
      moving_limit = player_controller->walk_speed;
    }

    rotation_speed = game->sensetivity * player_controller->rotation_speed;
    if ( input->mouse.right.current )
    {
      moving_limit *= 0.5;
      rotation_speed *= 0.75;
    }

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

      if ( game->physics.simulation_enabled )
      {
        if ( character_body->on_floor )
        {
          velocity = XMVectorSetY( velocity, 0.f );
        }
        else
        {
          velocity = XMVectorAdd( velocity, XMVectorScale( XMVectorSet( 0, -9.8, 0, 1 ), it->delta_time * player_controller->weight ) );
        }
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
          rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( basis_pivot_right, -rotation_speed * input->mouse.rel.y ) );
          rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( g_XMIdentityR1, -rotation_speed * input->mouse.rel.x ) );
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
  CRUDE_PROFILER_ZONE_END( "crude_player_controller_update_system_" );
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_player_controller_system )
{
  ECS_MODULE( world, crude_player_controller_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_player_controller_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_player_controller ) },
    { .id = ecs_id( crude_player ) }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_player_controller_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_player_controller ) }
  } );
}