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
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_player_controller )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( "crude_player_controller" ) );
  cJSON_AddItemToObject( free_camera_json, "rotate_speed", cJSON_CreateNumber( component->rotate_speed ) );
  cJSON_AddItemToObject( free_camera_json, "walk_speed", cJSON_CreateNumber( component->walk_speed ) );
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
    ImGui::DragFloat( "##Walk Speed", &component->walk_speed, 0.1f, 0.f, 0.f, "%.3f", ImGuiSliderFlags_ColorMarkers );
  } );
}

/**********************************************************
 *
 *                 API
 *
 *********************************************************/
static void
crude_player_health_death_callback_
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
);

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_OBSERVER_DECLARE( crude_player_controller_create_observer );
CRUDE_ECS_SYSTEM_DECLARE( crude_player_controller_game_update_system_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_player_controller_engine_update_system_ );

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

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_player_controller_engine_update_system_, crude_ecs_on_engine_update, ctx, {
    { .id = ecs_id( crude_player_controller ) },
  } );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_player_controller_game_update_system_, crude_ecs_on_game_update, ctx, {
    { .id = ecs_id( crude_player_controller ) },
    { .id = ecs_id( crude_transform ) },
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
    XMMATRIX                                               right_hand_joint_node_to_model;
    crude_entity                                           entity;  
    crude_entity                                           player_character_entity;
    crude_entity                                           player_orientation_entity;
    crude_entity                                           player_model_entity;
    crude_entity                                           weapon_entity;

    input = ctx->input;

    entity = crude_entity_from_iterator( it, i );

    player_controller = &player_controller_per_entity[ i ];
    
    player_character_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "character" );
    player_orientation_entity = crude_ecs_lookup_entity_from_parent( it->world, player_character_entity, "orientation" );
    player_model_entity = crude_ecs_lookup_entity_from_parent( it->world, player_orientation_entity, "model" );
    weapon_entity = crude_ecs_lookup_entity_from_parent( it->world, player_orientation_entity, "weapon" );

    player_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_model_entity, crude_gltf );
    
    player_controller->idle_animation_index = 0;
    player_controller->walk_animation_index = 1;
    player_controller->aim_down_animation_index = 4;

    player_model->model_renderer_resources_instance.animations_instances[ player_controller->idle_animation_index ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name(
      &player_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "idle" );
    player_model->model_renderer_resources_instance.animations_instances[ player_controller->walk_animation_index ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name(
      &player_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "walk" );
    player_model->model_renderer_resources_instance.animations_instances[ player_controller->aim_down_animation_index ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name(
      &player_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "aim_down" );

    player_model->model_renderer_resources_instance.animations_instances[ player_controller->idle_animation_index ].disabled = false;
    player_model->model_renderer_resources_instance.animations_instances[ player_controller->walk_animation_index ].disabled = false;
    player_model->model_renderer_resources_instance.animations_instances[ player_controller->aim_down_animation_index ].disabled = false;
    player_model->model_renderer_resources_instance.animations_instances[ player_controller->aim_down_animation_index ].loop = false;
    player_model->model_renderer_resources_instance.animations_instances[ player_controller->aim_down_animation_index ].current_time = 0.1;
    player_model->model_renderer_resources_instance.animations_instances[ player_controller->aim_down_animation_index ].paused = true;

    player_controller->head_joint_node = crude_gfx_model_renderer_resources_instance_find_node_by_name(
      &game->engine->model_renderer_resources_manager,
      &player_model->model_renderer_resources_instance,
      "mixamorig:Head" );

    player_controller->spine_joint_node = crude_gfx_model_renderer_resources_instance_find_node_by_name(
      &game->engine->model_renderer_resources_manager,
      &player_model->model_renderer_resources_instance,
      "mixamorig:Spine2" );

    player_controller->right_hand_joint_node = crude_gfx_model_renderer_resources_instance_find_node_by_name(
      &game->engine->model_renderer_resources_manager,
      &player_model->model_renderer_resources_instance,
      "mixamorig:RightHand" );
    
    player_controller->aim_blend = 0.f;
    player_controller->shot_blend = 0.f;
    player_controller->move_speed = 0.f;

    game->player_node = it->entities[ i ];
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_player_controller_game_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_player_controller_game_update_system_" );

  crude_game                                              *game;
  crude_player_controller_system_context                  *ctx;
  crude_player_controller                                 *player_controller_per_entity;
  crude_transform                                         *player_transform_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_player_controller_system_context*, it->ctx );
  player_controller_per_entity = ecs_field( it, crude_player_controller, 0 );
  player_transform_per_entity = ecs_field( it, crude_transform, 1 );
  
  if ( !crude_platform_cursor_hidden( &game->engine->platform ) )
  {
    crude_platform_hide_cursor( &game->engine->platform );
  }

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_player_controller                               *player_controller;
    crude_transform                                       *player_transform;
    crude_gltf                                            *player_model;
    crude_gltf                                            *weapon_model;
    crude_camera                                          *player_camera;
    crude_weapon                                          *weapon;
    crude_transform                                       *player_camera_transform;
    crude_entity                                           entity;  
    crude_entity                                           player_character_entity;
    crude_entity                                           camera_grab_point_entity;
    crude_entity                                           camera_front_point_entity;
    crude_entity                                           player_orientation_entity;
    crude_entity                                           player_model_entity;
    crude_entity                                           pivot_pitch_entity;
    crude_entity                                           pivot_yaw_entity;
    crude_entity                                           player_camera_entity;
    crude_entity                                           weapon_entity, weapon_model_entity, weapon_grab_entity, weapon_spawnpoint_entity;

    input = ctx->input;

    entity = crude_entity_from_iterator( it, i );

    player_controller = &player_controller_per_entity[ i ];
    player_transform = &player_transform_per_entity[ i ];
    
    player_character_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "character" );
    player_orientation_entity = crude_ecs_lookup_entity_from_parent( it->world, player_character_entity, "orientation" );
    player_model_entity = crude_ecs_lookup_entity_from_parent( it->world, player_orientation_entity, "model" );
    pivot_yaw_entity = crude_ecs_lookup_entity_from_parent( it->world, player_character_entity, "pivot_yaw" );
    pivot_pitch_entity = crude_ecs_lookup_entity_from_parent( it->world, pivot_yaw_entity, "pivot_pitch" );
    player_camera_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "camera" );
    weapon_grab_entity = crude_ecs_lookup_entity_from_parent( it->world, player_orientation_entity, "weapon_grab" );
    weapon_spawnpoint_entity = crude_ecs_lookup_entity_from_parent( it->world, weapon_grab_entity, "weapon_spawnpoint" );
    camera_grab_point_entity = crude_ecs_lookup_entity_from_parent( it->world, weapon_spawnpoint_entity, "camera_grab_point" );
    camera_front_point_entity = crude_ecs_lookup_entity_from_parent( it->world, pivot_pitch_entity, "camera_front_point" );
    weapon_entity = crude_ecs_lookup_entity_from_parent( it->world, weapon_spawnpoint_entity, "weapon" );
    weapon_model_entity = crude_ecs_lookup_entity_from_parent( it->world, weapon_entity, "model" );

    weapon = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, weapon_entity, crude_weapon );
    weapon_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, weapon_model_entity, crude_gltf );
    player_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_model_entity, crude_gltf );
    player_camera = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_camera_entity, crude_camera );
    player_camera_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_camera_entity, crude_transform );

    player_controller->camera_target_node = camera_front_point_entity;

    if ( player_controller->input_enabled )
    {
      /* Handle actions */
      {
        if ( input->mouse.right.current )
        {
          player_camera->fov_radians = crude_lerp_angle( player_camera->fov_radians, XMConvertToRadians( 90.f ), 2 * it->delta_time ); 
          player_controller->aim_blend = CRUDE_LERP( player_controller->aim_blend, input->mouse.right.current, 5 * it->delta_time );
          player_controller->camera_target_node = camera_grab_point_entity;
        }
        else
        {
          player_camera->fov_radians = crude_lerp_angle( player_camera->fov_radians, XMConvertToRadians( 70.f ), 2 * it->delta_time );
          player_controller->aim_blend = CRUDE_LERP( player_controller->aim_blend, input->mouse.right.current, 7 * it->delta_time );
        }
        
        if ( input->mouse.right.current )
        {
          if ( input->mouse.left.current )
          {
            if ( crude_weapon_fire( weapon_entity, weapon ) )
            {
              player_controller->shot_blend = 1.f;
            }
          }
        }
        player_controller->shot_blend = CRUDE_LERP( player_controller->shot_blend, 0, it->delta_system_time );
      }

      /* Handle Rotation */
      {
        crude_transform                                   *pivot_yaw_transform;
        crude_transform                                   *pivot_pitch_transform;
        crude_transform                                   *player_orientation_transform;
        XMVECTOR                                           yaw_rotation, pitch_rotation, pivot_yaw_rotation, pivot_pitch_rotation, player_orientation_rotation;

        pivot_pitch_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, pivot_pitch_entity, crude_transform );
        pivot_yaw_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, pivot_yaw_entity, crude_transform );
        player_orientation_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_orientation_entity, crude_transform );
        
        player_controller->pivot_yaw_angle -= CRUDE_CLAMP( player_controller->rotate_speed * input->mouse.rel.x, it->delta_time * 8.f, it->delta_time * -8.f );

        if ( player_controller->pivot_yaw_angle < 0 )
        {
          player_controller->pivot_yaw_angle += XM_2PI;
        }
        else if ( player_controller->pivot_yaw_angle >= XM_2PI )
        {
          player_controller->pivot_yaw_angle -= XM_2PI;
        }

        yaw_rotation = XMQuaternionRotationRollPitchYaw( 0.f, player_controller->pivot_yaw_angle, 0.f );
        pivot_yaw_rotation = XMLoadFloat4( &pivot_yaw_transform->rotation );
        pivot_yaw_rotation = XMQuaternionSlerp( pivot_yaw_rotation, yaw_rotation, 30.0 * it->delta_time );
        XMStoreFloat4( &pivot_yaw_transform->rotation, pivot_yaw_rotation );
        
        player_orientation_rotation = XMLoadFloat4( &player_orientation_transform->rotation );
        player_orientation_rotation = XMQuaternionSlerp( player_orientation_rotation, yaw_rotation, 10.0 * it->delta_time );
        XMStoreFloat4( &player_orientation_transform->rotation, player_orientation_rotation );
        
        player_controller->head_yaw_angle = crude_lerp_angle( player_controller->head_yaw_angle, player_controller->pivot_yaw_angle, CRUDE_MIN( 40.0 * it->delta_time, 1.f ) );
        player_controller->spine_yaw_angle = crude_lerp_angle( player_controller->spine_yaw_angle, player_controller->pivot_yaw_angle, CRUDE_MIN( 25.0 * it->delta_time, 1.f ) );
      }

      /* Handle movement */
      {
        crude_transform                                   *pivot_yaw_transform;
        crude_physics_character_container                 *physcs_character_container;
        XMMATRIX                                           pivot_yaw_to_world;
        XMVECTOR                                           pivot_yaw_basis_forward;
        XMVECTOR                                           player_velocity, new_player_velocity;
        XMFLOAT3                                           move_direction;

        move_direction.x = input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current;
        move_direction.y = input->keys[ SDL_SCANCODE_E ].current - input->keys[ SDL_SCANCODE_Q ].current;
        move_direction.z = input->keys[ SDL_SCANCODE_W ].current - input->keys[ SDL_SCANCODE_S ].current;
        
        pivot_yaw_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, pivot_yaw_entity, crude_transform );
        pivot_yaw_to_world = crude_transform_node_to_world( it->world, pivot_yaw_entity, pivot_yaw_transform );

        physcs_character_container = crude_physics_access_character( ctx->physics_manager, *CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_character_entity, crude_physics_character_handle ) );
      
        pivot_yaw_basis_forward = XMVector3Normalize( pivot_yaw_to_world.r[ 2 ] );
      
        player_controller->move_speed = player_controller->walk_speed;
        
        player_velocity = crude_jph_vec3_to_vector( physcs_character_container->jph_character_class->GetLinearVelocity( ) );
        
        if ( move_direction.z < 0 )
        {
          player_controller->move_speed *= 0.32;
        }

        if ( input->mouse.right.current )
        {
          player_controller->move_speed *= 0;
        }

        player_controller->move_speed *= move_direction.z;

        if ( move_direction.z != 0 )
        {
          new_player_velocity = XMVector3Normalize( pivot_yaw_basis_forward );
          new_player_velocity = XMVectorScale( new_player_velocity, player_controller->move_speed );
        
          new_player_velocity = XMVectorSet( XMVectorGetX( new_player_velocity ), XMVectorGetY( player_velocity ), XMVectorGetZ( new_player_velocity ), 1.f );
          
          player_model->model_renderer_resources_instance.animations_instances[ player_controller->walk_animation_index ].speed = move_direction.z < 0 ? 1.2 : -0.85;
        }
        else
        {
          new_player_velocity = player_velocity;
          new_player_velocity = XMVectorLerp( new_player_velocity, XMVectorZero( ), 5 * it->delta_time );
          new_player_velocity = XMVectorSetY( new_player_velocity, XMVectorGetY( player_velocity ) );
        }

        physcs_character_container->jph_character_class->SetLinearVelocity( crude_vector_to_jph_vec3( new_player_velocity ) );
      }
    }

    /* Handle camera */
    if ( player_controller->camera_enabled )
    {
      XMMATRIX                                             camera_target_to_world, world_to_player, camera_target_to_player;

      game->engine->camera_node = player_camera_entity;

      camera_target_to_world = crude_transform_node_to_world( it->world, player_controller->camera_target_node, NULL );
      world_to_player = XMMatrixInverse( NULL, crude_transform_node_to_world( it->world, entity, player_transform ) );
      camera_target_to_player = XMMatrixMultiply( camera_target_to_world, world_to_player );
      crude_transform_decompose( player_camera_transform, camera_target_to_player );
    }
    
    /* Handle animations */
    {
      int64                                                animation_indices[ 8 ] { -1 };
      float32                                              animation_weights[ 8 ]{ 0 };
      
      player_controller->move_blend = CRUDE_LERP( player_controller->move_blend, player_controller->move_speed / player_controller->walk_speed, 5 * it->delta_time );

      animation_indices[ 0 ] = player_controller->idle_animation_index;
      animation_weights[ 0 ] = 1.f - player_controller->move_blend;
      animation_indices[ 1 ] = player_controller->walk_animation_index;
      animation_weights[ 1 ] = player_controller->move_blend;
      animation_indices[ 2 ] = player_controller->aim_down_animation_index;
      animation_weights[ 2 ] = 2 * player_controller->aim_blend;
      crude_gfx_model_renderer_resources_instance_blend_animations( &player_model->model_renderer_resources_instance, animation_indices, animation_weights );
    }

    /* Handle bones */
    {
      crude_transform                                   *player_orientation_transform;
      XMVECTOR                                           head_rotation, spine_rotation;
      
      player_orientation_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_orientation_entity, crude_transform );
      
      if ( input->mouse.right.current )
      {
        head_rotation = XMQuaternionRotationRollPitchYaw( 0, player_controller->head_yaw_angle + 0.43 * XM_PIDIV2 * player_controller->aim_blend, 0.f );
        head_rotation = XMQuaternionMultiply( head_rotation, XMQuaternionConjugate( XMLoadFloat4( &player_orientation_transform->rotation ) ) );
        XMStoreFloat4( &player_model->model_renderer_resources_instance.nodes_transforms[ player_controller->head_joint_node ].rotation, head_rotation );
        
        spine_rotation = XMQuaternionRotationRollPitchYaw( 0.f, player_controller->spine_yaw_angle - 0.43 * XM_PIDIV2 * player_controller->aim_blend, 0.5 * XM_PIDIV2 * player_controller->shot_blend );
        spine_rotation = XMQuaternionMultiply( spine_rotation, XMQuaternionConjugate( XMLoadFloat4( &player_orientation_transform->rotation ) ) );
        XMStoreFloat4( &player_model->model_renderer_resources_instance.nodes_transforms[ player_controller->spine_joint_node ].rotation, spine_rotation );
      }
      else
      {
        head_rotation = XMQuaternionRotationRollPitchYaw(  0.f, player_controller->head_yaw_angle, 0.f );
        head_rotation = XMQuaternionMultiply( head_rotation, XMQuaternionConjugate( XMLoadFloat4( &player_orientation_transform->rotation ) ) );
        XMStoreFloat4( &player_model->model_renderer_resources_instance.nodes_transforms[ player_controller->head_joint_node ].rotation, head_rotation );
        
        spine_rotation = XMQuaternionRotationRollPitchYaw( 0.f, player_controller->spine_yaw_angle, 0.5 * XM_PIDIV2 * player_controller->shot_blend );
        spine_rotation = XMQuaternionMultiply( spine_rotation, XMQuaternionConjugate( XMLoadFloat4( &player_orientation_transform->rotation ) ) );
        XMStoreFloat4( &player_model->model_renderer_resources_instance.nodes_transforms[ player_controller->spine_joint_node ].rotation, spine_rotation );
      }
    }
  }
  CRUDE_PROFILER_ZONE_END;
}


void
crude_player_controller_engine_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_player_controller_engine_update_system_" );

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
    crude_transform                                       *weapon_grab_transform;
    XMMATRIX                                               right_hand_joint_node_to_model;
    crude_entity                                           entity;  
    crude_entity                                           player_character_entity;
    crude_entity                                           player_orientation_entity;
    crude_entity                                           player_model_entity;
    crude_entity                                           weapon_spawnpoint_entity, weapon_grab_entity;

    input = ctx->input;

    entity = crude_entity_from_iterator( it, i );

    player_controller = &player_controller_per_entity[ i ];
    
    player_character_entity = crude_ecs_lookup_entity_from_parent( it->world, entity, "character" );
    player_orientation_entity = crude_ecs_lookup_entity_from_parent( it->world, player_character_entity, "orientation" );
    player_model_entity = crude_ecs_lookup_entity_from_parent( it->world, player_orientation_entity, "model" );
    weapon_grab_entity = crude_ecs_lookup_entity_from_parent( it->world, player_orientation_entity, "weapon_grab" );
    weapon_spawnpoint_entity = crude_ecs_lookup_entity_from_parent( it->world, weapon_grab_entity, "weapon_spawnpoint" );

    player_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, player_model_entity, crude_gltf );
    
    weapon_grab_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, weapon_grab_entity, crude_transform );
    right_hand_joint_node_to_model = crude_gfx_node_to_model(
      crude_gfx_model_renderer_resources_manager_access_model_renderer_resources(
        &game->engine->model_renderer_resources_manager,
        player_model->model_renderer_resources_instance.model_renderer_resources_handle )->nodes,
      player_model->model_renderer_resources_instance.nodes_transforms,
      player_controller->right_hand_joint_node );

    XMVECTOR t, s, r;
    XMMatrixDecompose( &s, &r, &t, right_hand_joint_node_to_model );
    XMStoreFloat3( &weapon_grab_transform->translation, XMVectorLerp( XMLoadFloat3( &weapon_grab_transform->translation ), XMVectorScale( t, 25 ), 35 * it->delta_time ) );
    XMStoreFloat4( &weapon_grab_transform->rotation, XMVectorLerp( XMLoadFloat4( &weapon_grab_transform->rotation ), r, 35 * it->delta_time ) );
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_player_health_death_callback_
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
)
{
  crude_engine_commands_manager_push_load_node_command( &crude_game_instance()->engine->commands_manager, "" );
}