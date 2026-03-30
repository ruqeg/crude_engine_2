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
  component->rotate_speed = cJSON_GetNumberValue( cJSON_GetObjectItem( component_json, "rotate_speed" ) );
  component->walk_speed = cJSON_GetNumberValue( cJSON_GetObjectItem( component_json, "walk_speed" ) );
  component->pitch_limit = cJSON_GetNumberValue( cJSON_GetObjectItem( component_json, "pitch_limit" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_player_controller )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( "crude_player_controller" ) );
  cJSON_AddItemToObject( free_camera_json, "rotate_speed", cJSON_CreateNumber( component->rotate_speed ) );
  cJSON_AddItemToObject( free_camera_json, "walk_speed", cJSON_CreateNumber( component->walk_speed ) );
  cJSON_AddItemToObject( free_camera_json, "pitch_limit", cJSON_CreateNumber( component->pitch_limit ) );
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
  CRUDE_IMGUI_OPTION( "Rotate Speed", {
    ImGui::DragFloat( "##Rotate Speed", &component->rotate_speed, 0.1 );
  } );
  CRUDE_IMGUI_OPTION( "Walk Speed", {
    ImGui::DragFloat( "##Walk Speed", &component->walk_speed, 0.1 );
  } );
  CRUDE_IMGUI_OPTION( "Pitch Limit", {
    ImGui::SliderAngle( "##Pitch Limit", &component->pitch_limit, 15, 90 );
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

  crude_game                                              *game;
  crude_player_controller_system_context                  *ctx;
  crude_player_controller                                 *player_controller_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_player_controller_system_context*, it->ctx );
  player_controller_per_entity = ecs_field( it, crude_player_controller, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_player_controller                               *player_controller;
    crude_gltf                                            *player_model;
    crude_entity                                           entity;  
    crude_entity                                           player_world_transform_entity;
    crude_entity                                           player_model_entity;
    crude_entity                                           pivot_pitch_entity;
    crude_entity                                           pivot_yaw_entity;
    crude_entity                                           player_camera_entity;

    input = ctx->input;

    entity = crude_entity_from_iterator( it, i );

    player_controller = &player_controller_per_entity[ i ];
    
    player_world_transform_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "world_transform" );
    player_model_entity = crude_ecs_lookup_entity_from_parent( it->world, player_world_transform_entity, "model" );
    pivot_yaw_entity = crude_ecs_lookup_entity_from_parent( it->world, player_world_transform_entity, "pivot_yaw" );
    pivot_pitch_entity = crude_ecs_lookup_entity_from_parent( it->world, pivot_yaw_entity, "pivot_pitch" );
    player_camera_entity = crude_ecs_lookup_entity_from_parent( it->world, pivot_pitch_entity, "camera" );

    if ( player_controller->input_enabled )
    {
      /* Handle Rotation */
      {
        crude_transform                                   *pivot_yaw_transform;
        crude_transform                                   *pivot_pitch_transform;
        XMVECTOR                                           axis;
        float32                                            pivot_yaw_angle, pivot_pitch_angle;

        pivot_pitch_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, pivot_pitch_entity, crude_transform );
        pivot_yaw_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, pivot_yaw_entity, crude_transform );
        

        XMQuaternionToAxisAngle( &axis, &pivot_yaw_angle, XMLoadFloat4( &pivot_yaw_transform->rotation ) );
        XMQuaternionToAxisAngle( &axis, &pivot_pitch_angle, XMLoadFloat4( &pivot_pitch_transform->rotation ) );

        pivot_pitch_angle += player_controller->rotate_speed * input->mouse.rel.y;
        pivot_yaw_angle -= player_controller->rotate_speed * input->mouse.rel.x;

        if ( pivot_yaw_angle < 0 )
        {
          pivot_yaw_angle += XM_2PI;
        }
        else if ( pivot_yaw_angle >= XM_2PI )
        {
          pivot_yaw_angle -= XM_2PI;
        }

        if ( pivot_pitch_angle < 0 )
        {
          pivot_pitch_angle += XM_2PI;
        }
        else if ( pivot_pitch_angle >= XM_2PI )
        {
          pivot_pitch_angle -= XM_2PI;
        }

        if ( pivot_pitch_angle > player_controller->pitch_limit && pivot_pitch_angle < XM_2PI - player_controller->pitch_limit )
        {
          pivot_pitch_angle = pivot_pitch_angle > XM_PI ? ( XM_2PI - player_controller->pitch_limit ) : player_controller->pitch_limit;
        }

        XMStoreFloat4( &pivot_yaw_transform->rotation, XMQuaternionRotationRollPitchYaw( 0.f, pivot_yaw_angle, 0.f ) );
        XMStoreFloat4( &pivot_pitch_transform->rotation, XMQuaternionRotationRollPitchYaw( pivot_pitch_angle, 0.f, 0.f ) );
      }

      /* Handle movement */
      {
        crude_transform                                   *player_world_transform;
        crude_transform                                   *player_camera_transform;
        XMMATRIX                                           player_camera_to_world;
        XMVECTOR                                           player_world_translation;
        XMVECTOR                                           player_camera_basis_right, player_camera_basis_forward, player_camera_basis_up;
        XMFLOAT3                                           move_direction;
        float32                                            move_speed;
      
        move_direction.x = input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current;
        move_direction.y = input->keys[ SDL_SCANCODE_E ].current - input->keys[ SDL_SCANCODE_Q ].current;
        move_direction.z = input->keys[ SDL_SCANCODE_S ].current - input->keys[ SDL_SCANCODE_W ].current;
        
        player_camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_camera_entity, crude_transform );
        player_camera_to_world = crude_transform_node_to_world( it->world, player_camera_entity, player_camera_transform );

        player_world_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_world_transform_entity, crude_transform );
      
        player_camera_basis_right = XMVector3Normalize( player_camera_to_world.r[ 0 ] );
        player_camera_basis_up = XMVector3Normalize( player_camera_to_world.r[ 1 ] );
        player_camera_basis_forward = XMVector3Normalize( player_camera_to_world.r[ 2 ] );
      
        move_speed = player_controller->walk_speed;

        //if ( input->keys[ SDL_SCANCODE_LSHIFT ].current )
        //{
        //  moving_speed = moving_speed * 2.f;
        //}
        
        player_world_translation = XMLoadFloat3( &player_world_transform->translation );

        if ( move_direction.x != 0.f )
        {
          player_world_translation = XMVectorAdd( player_world_translation, XMVectorScale( player_camera_basis_right, move_speed * move_direction.x * it->delta_time ) );
        }
        if ( move_direction.y != 0.f )
        {
          player_world_translation = XMVectorAdd( player_world_translation, XMVectorScale( player_camera_basis_up, move_speed * move_direction.y * it->delta_time ) );
        }
        if ( move_direction.z != 0.f )
        {
          player_world_translation = XMVectorAdd( player_world_translation, XMVectorScale( player_camera_basis_forward, move_speed * move_direction.z * it->delta_time ) );
        }

        XMStoreFloat3( &player_world_transform->translation, player_world_translation );
      }
    }
    
    if ( player_controller->camera_enabled )
    {
      game->engine->camera_node = player_camera_entity;
    }
    
    player_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_model_entity, crude_gltf );
    crude_gfx_model_renderer_resources_instance_set_animation_by_name( &player_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "Armature|mixamo.com|Layer0" ); 
    player_model->model_renderer_resources_instance.animation_instance.loop = true;
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

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_player_controller_update_system_, crude_ecs_on_game_update, ctx, {
    { .id = ecs_id( crude_player_controller ) },
  } );
}