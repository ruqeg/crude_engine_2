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
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_player_controller )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( "crude_player_controller" ) );
  cJSON_AddItemToObject( free_camera_json, "rotate_speed", cJSON_CreateNumber( component->rotate_speed ) );
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
    crude_entity                                           player_model_entity;
    crude_entity                                           pivot_center_entity;
    crude_entity                                           pivot_z_offset_entity;
    crude_entity                                           camera_entity;

    input = ctx->input;

    entity = crude_entity_from_iterator( it, i );

    player_controller = &player_controller_per_entity[ i ];
    
    pivot_center_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "pivot_center" );
    pivot_z_offset_entity = crude_ecs_lookup_entity_from_parent( it->world, pivot_center_entity, "pivot_z_offset" );
    camera_entity = crude_ecs_lookup_entity_from_parent( it->world, pivot_z_offset_entity, "camera" );
    player_model_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "model" );

    if ( player_controller->input_enabled )
    {
      crude_transform                                     *pivot_center_transform;
      crude_transform                                     *pivot_z_offset_transform;
      XMVECTOR                                             pivot_z_offset_rotation;
      XMVECTOR                                             pivot_center_rotation, pivot_center_camera_up, pivot_center_basis_up, pivot_center_basis_right;
      XMMATRIX                                             pivot_center_to_world;

      pivot_center_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, pivot_center_entity, crude_transform );
      pivot_z_offset_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, pivot_z_offset_entity, crude_transform );


      //int32 moving_forward = input->keys[ SDL_SCANCODE_W ].current - input->keys[ SDL_SCANCODE_S ].current;
      //int32 moving_up = input->keys[ SDL_SCANCODE_E ].current - input->keys[ SDL_SCANCODE_Q ].current;
      //int32 moving_right = input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current;
      //
      //XMVECTOR translation = XMLoadFloat3( &transforms_per_entity[ i ].translation );
      //XMMATRIX node_to_world = crude_transform_node_to_world( it->world, crude_entity_from_iterator( it, i ), &transforms_per_entity[ i ] );
      //
      //XMVECTOR basis_right = XMVector3Normalize( node_to_world.r[ 0 ] );
      //XMVECTOR basis_up = XMVector3Normalize( node_to_world.r[ 1 ] );
      //XMVECTOR basis_forward = XMVector3Normalize( node_to_world.r[ 2 ] );
      //
      //float32 moving_speed = player_controller_per_entity[ i ].moving_speed_multiplier;
      //if ( input->keys[ SDL_SCANCODE_LSHIFT ].current )
      //{
      //  moving_speed = moving_speed * 2.f;
      //}
      //
      //if ( moving_right )
      //{
      //  translation = XMVectorAdd( translation, XMVectorScale( basis_right, moving_speed * moving_right * it->delta_time * 1.f ) );
      //}
      //if ( moving_forward )
      //{
      //  translation = XMVectorAdd( translation, XMVectorScale( basis_forward, moving_speed * moving_forward * it->delta_time * -1.f ) );
      //}
      //if ( moving_up )
      //{
      //  translation = XMVectorAdd( translation, XMVectorScale( basis_up, moving_speed * moving_up * it->delta_time * 1.f ) );
      //}
      //XMStoreFloat3( &transforms_per_entity[ i ].translation, translation );
      //
      //if ( input->mouse.right.current )
      //{
      //  XMVECTOR rotation = XMLoadFloat4( &transforms_per_entity[ i ].rotation );
      //  XMVECTOR camera_up = XMVectorGetY( basis_up ) > 0.0f ? g_XMIdentityR1 : XMVectorNegate( g_XMIdentityR1 );
      //
      //  rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( basis_right, -player_controller_per_entity[ i ].rotating_speed_multiplier * input->mouse.rel.y ) );
      //  rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( camera_up, -player_controller_per_entity[ i ].rotating_speed_multiplier * input->mouse.rel.x ) );
      //  XMStoreFloat4( &transforms_per_entity[ i ].rotation, rotation );
      //}
      //translation = XMLoadFloat3( &transform->translation );
      pivot_center_to_world = crude_transform_node_to_world( it->world, pivot_center_entity, pivot_center_transform );

      pivot_center_basis_right = XMVector3Normalize( pivot_center_to_world.r[ 0 ] );
      pivot_center_basis_up = XMVector3Normalize( pivot_center_to_world.r[ 1 ] );
      
      pivot_center_camera_up = XMVectorGetY( pivot_center_basis_up ) > 0.0f ? g_XMIdentityR1 : XMVectorNegate( g_XMIdentityR1 );
      
      pivot_z_offset_rotation = XMLoadFloat4( &pivot_z_offset_transform->rotation );
      pivot_z_offset_rotation = XMQuaternionMultiply( pivot_z_offset_rotation, XMQuaternionRotationAxis( pivot_center_basis_right, -player_controller->rotate_speed * input->mouse.rel.y ) );
      XMStoreFloat4( &pivot_z_offset_transform->rotation, pivot_z_offset_rotation );

      //pivot_center_rotation = XMLoadFloat4( &pivot_center_transform->rotation );
      //pivot_center_rotation = XMQuaternionMultiply( pivot_center_rotation, XMQuaternionRotationAxis( pivot_center_camera_up, -player_controller->rotate_speed * input->mouse.rel.x ) );
      //XMStoreFloat4( &pivot_center_transform->rotation, pivot_center_rotation );
    }
    
    if ( player_controller->camera_enabled )
    {
      game->engine->camera_node = camera_entity;
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