#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/platform/platform.h>
#include <engine/graphics/imgui.h>
#include <game/game.h>

#include <game/player_controller_ecs.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_player_controller );
CRUDE_COMPONENT_STRING_DEFINE( crude_player_controller, "crude_player_controller" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_player_controller )
{
  crude_memory_set( component, 0, sizeof( crude_player_controller ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_player_controller )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( "crude_player_controller" ) );
  return free_camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_player_controller )
{
  CRUDE_IMGUI_START_OPTIONS;

  CRUDE_IMGUI_OPTION( "Camera Enabled", {
    ImGui::Checkbox( "##Camera Enabled", &component->camera_enabled );
  } );
  CRUDE_IMGUI_OPTION( "Input Enabled", {
    ImGui::Checkbox( "##Input Enabled", &component->input_enabled );
  } );
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_player_controller_update_system_ );

void
crude_player_controller_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_player_controller_update_system" );

  crude_game *game = crude_game_instance( );
  crude_player_controller_system_context *ctx = CRUDE_CAST( crude_player_controller_system_context*, it->ctx );
  crude_transform *transforms_per_entity = ecs_field( it, crude_transform, 0 );
  crude_player_controller *player_controller_per_entity = ecs_field( it, crude_player_controller, 1 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_player_controller                               *player_controller;
      
    input = ctx->input;

    player_controller = &player_controller_per_entity[ i ];
    if ( player_controller->input_enabled )
    {
      int32 moving_forward = input->keys[ SDL_SCANCODE_W ].current - input->keys[ SDL_SCANCODE_S ].current;
      int32 moving_up = input->keys[ SDL_SCANCODE_E ].current - input->keys[ SDL_SCANCODE_Q ].current;
      int32 moving_right = input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current;

      XMVECTOR translation = XMLoadFloat3( &transforms_per_entity[ i ].translation );
      XMMATRIX node_to_world = crude_transform_node_to_world( it->world, crude_entity_from_iterator( it, i ), &transforms_per_entity[ i ] );

      XMVECTOR basis_right = XMVector3Normalize( node_to_world.r[ 0 ] );
      XMVECTOR basis_up = XMVector3Normalize( node_to_world.r[ 1 ] );
      XMVECTOR basis_forward = XMVector3Normalize( node_to_world.r[ 2 ] );
      
      float32 moving_speed = player_controller_per_entity[ i ].moving_speed_multiplier;
      if ( input->keys[ SDL_SCANCODE_LSHIFT ].current )
      {
        moving_speed = moving_speed * 2.f;
      }

      if ( moving_right )
      {
        translation = XMVectorAdd( translation, XMVectorScale( basis_right, moving_speed * moving_right * it->delta_time * 1.f ) );
      }
      if ( moving_forward )
      {
        translation = XMVectorAdd( translation, XMVectorScale( basis_forward, moving_speed * moving_forward * it->delta_time * -1.f ) );
      }
      if ( moving_up )
      {
        translation = XMVectorAdd( translation, XMVectorScale( basis_up, moving_speed * moving_up * it->delta_time * 1.f ) );
      }
      XMStoreFloat3( &transforms_per_entity[ i ].translation, translation );

      if ( input->mouse.right.current )
      {
        XMVECTOR rotation = XMLoadFloat4( &transforms_per_entity[ i ].rotation );
        XMVECTOR camera_up = XMVectorGetY( basis_up ) > 0.0f ? g_XMIdentityR1 : XMVectorNegate( g_XMIdentityR1 );

        rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( basis_right, -player_controller_per_entity[ i ].rotating_speed_multiplier * input->mouse.rel.y ) );
        rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( camera_up, -player_controller_per_entity[ i ].rotating_speed_multiplier * input->mouse.rel.x ) );
        XMStoreFloat4( &transforms_per_entity[ i ].rotation, rotation );
      }
    }
    
    if ( player_controller->camera_enabled )
    {
      game->engine->camera_node = crude_ecs_lookup_entity_from_parent( it->world, it->entities[ i ], "camera" );
    }
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_player_controller_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_player_controller_system_context             *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_player_controller_system );
  
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_player_controller );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_player_controller );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_player_controller );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_player_controller );

  crude_scene_components_import( world, manager );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_player_controller_update_system_, EcsOnUpdate, ctx, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_player_controller ) },
  } );
}