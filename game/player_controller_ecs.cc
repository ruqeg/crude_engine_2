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
CRUDE_ECS_OBSERVER_DECLARE( crude_player_controller_create_observer );
CRUDE_ECS_SYSTEM_DECLARE( crude_player_controller_update_system_ );

void
crude_player_controller_create_observer
(
  _In_ ecs_iter_t                                         *it
);

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
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_player_controller_create_observer, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_player_controller ), .oper = EcsAnd }
  } );
}

void
crude_player_controller_create_observer
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_player_controller_create_observer" );

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
    crude_entity                                           player_character_entity;
    crude_entity                                           player_orientation_entity;
    crude_entity                                           player_model_entity;

    input = ctx->input;

    entity = crude_entity_from_iterator( it, i );

    player_controller = &player_controller_per_entity[ i ];
    
    player_character_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "character" );
    player_orientation_entity = crude_ecs_lookup_entity_from_parent( it->world, player_character_entity, "orientation" );
    player_model_entity = crude_ecs_lookup_entity_from_parent( it->world, player_orientation_entity, "model" );

    player_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_model_entity, crude_gltf );
    player_model->model_renderer_resources_instance.animations_instances[ 0 ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name( &player_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "idle" );
    player_model->model_renderer_resources_instance.animations_instances[ 1 ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name( &player_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "walk" );
    
    player_controller->head_joint_node = crude_gfx_model_renderer_resources_instance_find_node_by_name(
      &game->engine->model_renderer_resources_manager,
      &player_model->model_renderer_resources_instance,
      "mixamorig:Head" );

    player_controller->spine_joint_node = crude_gfx_model_renderer_resources_instance_find_node_by_name(
      &game->engine->model_renderer_resources_manager,
      &player_model->model_renderer_resources_instance,
      "mixamorig:Spine2" );
  }
  CRUDE_PROFILER_ZONE_END;
}

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
  
  if ( !crude_platform_cursor_hidden( &game->engine->platform ) )
  {
    crude_platform_hide_cursor( &game->engine->platform );
  }

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_player_controller                               *player_controller;
    crude_gltf                                            *player_model;
    crude_entity                                           entity;  
    crude_entity                                           player_character_entity;
    crude_entity                                           player_orientation_entity;
    crude_entity                                           player_model_entity;
    crude_entity                                           pivot_pitch_entity;
    crude_entity                                           pivot_yaw_entity;
    crude_entity                                           player_camera_entity;

    input = ctx->input;

    entity = crude_entity_from_iterator( it, i );

    player_controller = &player_controller_per_entity[ i ];
    
    player_character_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "character" );
    player_orientation_entity = crude_ecs_lookup_entity_from_parent( it->world, player_character_entity, "orientation" );
    player_model_entity = crude_ecs_lookup_entity_from_parent( it->world, player_orientation_entity, "model" );
    pivot_yaw_entity = crude_ecs_lookup_entity_from_parent( it->world, player_character_entity, "pivot_yaw" );
    pivot_pitch_entity = crude_ecs_lookup_entity_from_parent( it->world, pivot_yaw_entity, "pivot_pitch" );
    player_camera_entity = crude_ecs_lookup_entity_from_parent( it->world, pivot_pitch_entity, "camera" );

    if ( player_controller->input_enabled )
    {
      /* Handle Rotation */
      {
        crude_transform                                   *pivot_yaw_transform;
        crude_transform                                   *pivot_pitch_transform;
        crude_transform                                   *player_orientation_transform;
        XMVECTOR                                           yaw_rotation, pitch_rotation, pivot_yaw_rotation, pivot_pitch_rotation, player_orientation_rotation;
        float32                                            player_orientation_yaw_limit;

        pivot_pitch_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, pivot_pitch_entity, crude_transform );
        pivot_yaw_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, pivot_yaw_entity, crude_transform );
        player_orientation_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_orientation_entity, crude_transform );

        player_controller->pivot_pitch_angle += CRUDE_CLAMP( player_controller->rotate_speed * input->mouse.rel.y, 0.04f, -0.04f );
        player_controller->pivot_yaw_angle -= CRUDE_CLAMP( player_controller->rotate_speed * input->mouse.rel.x, 0.04f, -0.04f );

        if ( player_controller->pivot_yaw_angle < 0 )
        {
          player_controller->pivot_yaw_angle += XM_2PI;
        }
        else if ( player_controller->pivot_yaw_angle >= XM_2PI )
        {
          player_controller->pivot_yaw_angle -= XM_2PI;
        }

        if ( player_controller->pivot_pitch_angle < 0 )
        {
          player_controller->pivot_pitch_angle += XM_2PI;
        }
        else if ( player_controller->pivot_pitch_angle >= XM_2PI )
        {
          player_controller->pivot_pitch_angle -= XM_2PI;
        }

        if ( player_controller->pivot_pitch_angle > player_controller->pitch_limit && player_controller->pivot_pitch_angle < XM_2PI - player_controller->pitch_limit )
        {
          player_controller->pivot_pitch_angle = player_controller->pivot_pitch_angle > XM_PI ? ( XM_2PI - player_controller->pitch_limit ) : player_controller->pitch_limit;
        }

        player_orientation_yaw_limit = player_controller->pivot_yaw_angle;

        yaw_rotation = XMQuaternionRotationRollPitchYaw( 0.f, player_controller->pivot_yaw_angle, 0.f );
        pivot_yaw_rotation = XMLoadFloat4( &pivot_yaw_transform->rotation );
        pivot_yaw_rotation = XMQuaternionSlerp( pivot_yaw_rotation, yaw_rotation, 30.0 * it->delta_time );
        XMStoreFloat4( &pivot_yaw_transform->rotation, pivot_yaw_rotation );
        
        pitch_rotation = XMQuaternionRotationRollPitchYaw( player_controller->pivot_pitch_angle, 0.f, 0.f );
        pivot_pitch_rotation = XMLoadFloat4( &pivot_pitch_transform->rotation );
        pivot_pitch_rotation = XMQuaternionSlerp( pivot_pitch_rotation, pitch_rotation, 30.0 * it->delta_time );
        XMStoreFloat4( &pivot_pitch_transform->rotation, pivot_pitch_rotation );
        
        player_orientation_rotation = XMLoadFloat4( &player_orientation_transform->rotation );
        player_orientation_rotation = XMQuaternionSlerp( player_orientation_rotation, yaw_rotation, 10.0 * it->delta_time );
        XMStoreFloat4( &player_orientation_transform->rotation, player_orientation_rotation );

        player_controller->head_pitch_angle = player_controller->pivot_pitch_angle;
        player_controller->head_yaw_angle = crude_lerp_angle( player_controller->head_yaw_angle, player_controller->pivot_yaw_angle, CRUDE_MIN( 40.0 * it->delta_time, 1.f ) );
        player_controller->spine_yaw_angle = crude_lerp_angle( player_controller->spine_yaw_angle, player_controller->pivot_yaw_angle, CRUDE_MIN( 25.0 * it->delta_time, 1.f ) );
      }

      /* Handle movement */
      {
        crude_transform                                   *player_camera_transform;
        crude_physics_character_container                 *physcs_character_container;
        XMMATRIX                                           player_camera_to_world;
        XMVECTOR                                           player_camera_basis_right, player_camera_basis_forward, player_camera_basis_up;
        XMVECTOR                                           player_velocity, new_player_velocity;
        XMFLOAT3                                           move_direction;
        float32                                            move_speed, walk_blend_final;
      
        move_direction.x = input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current;
        move_direction.y = input->keys[ SDL_SCANCODE_E ].current - input->keys[ SDL_SCANCODE_Q ].current;
        move_direction.z = input->keys[ SDL_SCANCODE_S ].current - input->keys[ SDL_SCANCODE_W ].current;
        
        player_camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_camera_entity, crude_transform );
        player_camera_to_world = crude_transform_node_to_world( it->world, player_camera_entity, player_camera_transform );

        physcs_character_container = crude_physics_access_character( ctx->physics_manager, *CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_character_entity, crude_physics_character_handle ) );
      
        player_camera_basis_right = XMVector3Normalize( player_camera_to_world.r[ 0 ] );
        player_camera_basis_up = XMVector3Normalize( player_camera_to_world.r[ 1 ] );
        player_camera_basis_forward = XMVector3Normalize( player_camera_to_world.r[ 2 ] );
      
        move_speed = player_controller->walk_speed;

        //if ( input->keys[ SDL_SCANCODE_LSHIFT ].current )
        //{
        //  moving_speed = moving_speed * 2.f;
        //}
        
        player_velocity = crude_jph_vec3_to_vector( physcs_character_container->jph_character_class->GetLinearVelocity( ) );
        
        if ( move_direction.x != 0.f || move_direction.z != 0 )
        {
          new_player_velocity = XMVectorZero( );
          new_player_velocity = XMVectorAdd( new_player_velocity, XMVectorScale( player_camera_basis_right, move_direction.x ) );
          new_player_velocity = XMVectorAdd( new_player_velocity, XMVectorScale( player_camera_basis_forward, move_direction.z ) );
          new_player_velocity = XMVectorScale( XMVector3Normalize( new_player_velocity ), move_speed );
        
          new_player_velocity = XMVectorSet( XMVectorGetX( new_player_velocity ), XMVectorGetY( player_velocity ), XMVectorGetZ( new_player_velocity ), 1.f );

          walk_blend_final = XMVectorGetX( XMVector3Length( new_player_velocity ) ) / move_speed;
          player_controller->walk_blend = CRUDE_LERP( player_controller->walk_blend, walk_blend_final, 5 * it->delta_time  );
        }
        else
        {
          new_player_velocity = player_velocity;
          new_player_velocity = XMVectorLerp( new_player_velocity, XMVectorZero( ), 5 * it->delta_time );
          new_player_velocity = XMVectorSetY( new_player_velocity, XMVectorGetY( player_velocity ) );
          player_controller->walk_blend = CRUDE_LERP( player_controller->walk_blend, 0.f, 5 * it->delta_time  );
        }

        physcs_character_container->jph_character_class->SetLinearVelocity( crude_vector_to_jph_vec3( new_player_velocity ) );
      }
    }
    
    if ( player_controller->camera_enabled )
    {
      game->engine->camera_node = player_camera_entity;
    }
    
    player_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_model_entity, crude_gltf );

    player_model->model_renderer_resources_instance.animations_instances[ 0 ].disabled = false;
    player_model->model_renderer_resources_instance.animations_instances[ 1 ].disabled = false;

    crude_gfx_model_renderer_resources_instance_blend_animations( &player_model->model_renderer_resources_instance, 0, 1, player_controller->walk_blend );

    {
      crude_transform                                   *player_orientation_transform;
      XMVECTOR                                           head_rotation, spine_rotation;
      
      player_orientation_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_orientation_entity, crude_transform );
      
      head_rotation = XMQuaternionRotationRollPitchYaw( player_controller->head_pitch_angle, player_controller->head_yaw_angle, 0.f );
      head_rotation = XMQuaternionMultiply( head_rotation, XMQuaternionConjugate( XMLoadFloat4( &player_orientation_transform->rotation ) ) );
      XMStoreFloat4( &player_model->model_renderer_resources_instance.nodes_transforms[ player_controller->head_joint_node ].rotation, head_rotation );
      
      spine_rotation = XMQuaternionRotationRollPitchYaw( 0.f, player_controller->spine_yaw_angle, 0.f );
      spine_rotation = XMQuaternionMultiply( spine_rotation, XMQuaternionConjugate( XMLoadFloat4( &player_orientation_transform->rotation ) ) );
      XMStoreFloat4( &player_model->model_renderer_resources_instance.nodes_transforms[ player_controller->spine_joint_node ].rotation, spine_rotation );
    }
  }
  CRUDE_PROFILER_ZONE_END;
}