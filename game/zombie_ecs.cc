#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/platform/platform.h>
#include <engine/graphics/imgui.h>
#include <game/game.h>

#include <game/zombie_ecs.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_dead );
ECS_COMPONENT_DECLARE( crude_zombie );
CRUDE_COMPONENT_STRING_DEFINE( crude_zombie, "crude_zombie" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_zombie )
{
  crude_memory_set( component, 0, sizeof( crude_zombie ) );
  component->damage = cJSON_GetNumberValue( cJSON_GetObjectItem( component_json, "damage" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_zombie )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_zombie ) ) );
  cJSON_AddItemToObject( free_camera_json, "damage", cJSON_CreateNumber( component->damage ) );
  return free_camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_zombie )
{
  CRUDE_IMGUI_START_OPTIONS;

  CRUDE_IMGUI_OPTION( "Damage", {
    ImGui::InputFloat( "##Damage", &component->damage );
    })
}

/**********************************************************
 *
 *                 API
 *
 *********************************************************/
static void
crude_zombie_health_damage_callback_
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
);

static void
crude_zombie_health_death_callback_
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
);

static void
crude_zombie_attack_callback_
(
  _In_ crude_entity                                        signal_entity,
  _In_ crude_entity                                        hitted_entity
);

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_OBSERVER_DECLARE( crude_zombie_create_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_zombie_game_update_system_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_zombie_engine_update_system_ );

static void
crude_zombie_create_observer_
(
  _In_ ecs_iter_t                                         *it
);

void
crude_zombie_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_zombie_system_context                        *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_zombie_system );
  
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_zombie );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_dead );

  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_zombie );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_zombie );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_zombie );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_zombie_game_update_system_, crude_ecs_on_game_update, ctx, {
    { .id = ecs_id( crude_zombie ) },
    { .id = ecs_id( crude_dead ), .oper = EcsNot },
  } );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_zombie_engine_update_system_, crude_ecs_on_engine_update, ctx, {
    { .id = ecs_id( crude_zombie ) },
    { .id = ecs_id( crude_dead ), .oper = EcsNot },
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_zombie_create_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_zombie ), .oper = EcsAnd }
  } );

  crude_scene_components_import( world, manager );
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
void
crude_zombie_create_observer_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_zombie_create_observer_" );

  crude_game                                              *game;
  crude_zombie_system_context                             *ctx;
  crude_zombie                                            *zombie_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_zombie_system_context*, it->ctx );
  zombie_per_entity = ecs_field( it, crude_zombie, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_zombie                                          *zombie;
    crude_entity                                           zombie_entity;
    crude_entity                                           zombie_pivot_entity;

    zombie_entity = crude_entity_from_iterator( it, i );

    zombie = &zombie_per_entity[ i ];
    
    zombie->dying = false;

    zombie_pivot_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_entity, "pivot" );

    /* Health setup */
    {
      crude_health                                        *health;
      crude_entity                                         health_entity;
      crude_health_callback_container                      health_callback_container;

      health_callback_container.damage_callback = crude_zombie_health_damage_callback_;
      health_callback_container.death_callback = crude_zombie_health_death_callback_;

      health_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_entity, "health" );
      health = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, health_entity, crude_health );
      health->callback_container = health_callback_container;
    }

    /* Attack setup */
    {
      crude_physics_kinematic_body_container              *zombie_attack_sensor_kinematic_body_container;
      crude_entity                                         zombie_attack_sensor_entity;
      crude_physics_kinematic_body_handle                  zombie_attack_sensor_kinematic_body_handle;


      zombie_attack_sensor_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_pivot_entity, "attack_sensor" );
      zombie_attack_sensor_kinematic_body_handle = *CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( it->world, zombie_attack_sensor_entity, crude_physics_kinematic_body_handle );
      zombie_attack_sensor_kinematic_body_container = crude_physics_access_kinematic_body( &game->engine->physics, zombie_attack_sensor_kinematic_body_handle );
      zombie_attack_sensor_kinematic_body_container->contact_added_callback = crude_zombie_attack_callback_;
    }

    /* Model setup */
    {
      crude_gltf                                          *zombie_model;
      crude_entity                                         zombie_model_entity;
      
      zombie_model_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_pivot_entity, "model" );

      zombie_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, zombie_model_entity, crude_gltf );
      
      zombie->idle_animation_index = 0;
      zombie->hit_animation_index = 1;
      zombie->dead_animation_index = 2;
      zombie->walk_animation_index = 3;
      zombie->attack_animation_index = 4;

      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->idle_animation_index ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name(
        &zombie_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "crazy" );
      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->hit_animation_index ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name(
        &zombie_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "hit" );
      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->dead_animation_index ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name(
        &zombie_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "dead" );
      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->walk_animation_index ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name(
        &zombie_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "walk" );
      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->attack_animation_index ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name(
        &zombie_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "hit_hand" );

      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->idle_animation_index ].disabled = false;

      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->hit_animation_index ].disabled = true;
      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->hit_animation_index ].loop = false;

      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->dead_animation_index ].disabled = true;
      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->dead_animation_index ].loop = false;

      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->walk_animation_index ].disabled = false;

      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->attack_animation_index ].disabled = true;
      zombie_model->model_renderer_resources_instance.animations_instances[ zombie->attack_animation_index ].loop = false;
      
      zombie->spine_joint_node = crude_gfx_model_renderer_resources_instance_find_node_by_name(
        &game->engine->model_renderer_resources_manager,
        &zombie_model->model_renderer_resources_instance,
        "mixamorig:Spine" );

      zombie->target_point.y = 0.f;
    }
  }
  CRUDE_PROFILER_ZONE_END;
}


void
crude_zombie_game_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_zombie_game_update_system_" );
  
  crude_game                                              *game;
  crude_zombie_system_context                             *ctx;
  crude_zombie                                            *zombie_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_zombie_system_context*, it->ctx );
  zombie_per_entity = ecs_field( it, crude_zombie, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_zombie                                          *zombie;
    crude_transform                                       *pivot_transform;
    crude_transform                                       *zombie_transform;
    crude_gltf                                            *zombie_model;
    crude_entity                                           zombie_pivot_entity;
    crude_entity                                           zombie_model_entity;
    crude_entity                                           zombie_entity;

    zombie_entity = crude_entity_from_iterator( it, i );

    zombie = &zombie_per_entity[ i ];
    
    zombie_pivot_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_entity, "pivot" );
    zombie_model_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_pivot_entity, "model" );

    zombie_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, zombie_model_entity, crude_gltf );
    zombie_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, zombie_entity, crude_transform );
    pivot_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, zombie_pivot_entity, crude_transform );

    if ( zombie->dying )
    {
      if ( !zombie_model->model_renderer_resources_instance.animations_instances[ zombie->dead_animation_index ].disabled )
      {
        crude_gfx_model_renderer_resources_instance_blend_one_animation( &zombie_model->model_renderer_resources_instance, zombie->dead_animation_index );
      }
      else
      {
        CRUDE_ENTITY_ADD_COMPONENT( game->engine->world, zombie_entity, crude_dead );
      }
    }
    else
    {
      if ( zombie_model->model_renderer_resources_instance.animations_instances[ zombie->attack_animation_index ].disabled && zombie_model->model_renderer_resources_instance.animations_instances[ zombie->hit_animation_index ].disabled && zombie->target_point.y != 0 )
      {
        zombie_model->model_renderer_resources_instance.animations_instances[ zombie->walk_animation_index ].disabled = false;
      }
      else
      {
        zombie_model->model_renderer_resources_instance.animations_instances[ zombie->walk_animation_index ].disabled = true;
      }
      
      if ( zombie_model->model_renderer_resources_instance.animations_instances[ zombie->attack_animation_index ].disabled && zombie_model->model_renderer_resources_instance.animations_instances[ zombie->hit_animation_index ].disabled && ( zombie->target_point.y != 0 ) )
      {
        XMVECTOR                                           target_position;
        XMVECTOR                                           pivot_rotation;
        XMVECTOR                                           direction_to_taget;
        XMVECTOR                                           zombie_translation, zombie_new_translation;
        XMMATRIX                                           zombie_pivot_to_world;
        XMVECTOR                                           target_rotation;
        
        zombie_pivot_to_world = crude_transform_node_to_world( it->world, zombie_pivot_entity, NULL );
        target_position = XMLoadFloat3( &zombie->target_point );

        target_rotation = XMQuaternionRotationMatrix( XMMatrixLookAtRH( XMVectorSetY( target_position, 0 ), XMVectorSetY( zombie_pivot_to_world.r[ 3 ], 0 ), XMVectorSet( 0, 1, 0, 0 ) ) );
        
        pivot_rotation = XMLoadFloat4( &pivot_transform->rotation );
        pivot_rotation = XMQuaternionSlerp( pivot_rotation, XMQuaternionInverse( target_rotation ), 15 * it->delta_time );
        XMStoreFloat4( &pivot_transform->rotation, pivot_rotation);
        
        direction_to_taget = XMVector3Normalize( XMVectorSubtract( XMVectorSetY( target_position, 0 ), XMVectorSetY( zombie_pivot_to_world.r[ 3 ], 0 ) ) );

        zombie_translation = XMLoadFloat3( &zombie_transform->translation );
        zombie_new_translation = XMVectorAdd( zombie_translation, XMVectorScale( direction_to_taget, 5 * it->delta_time ) );
        zombie_new_translation = XMVectorSetY( zombie_new_translation, XMVectorGetY( zombie_translation ) );
        XMStoreFloat3( &zombie_transform->translation, zombie_new_translation );
      }

      if ( zombie_model->model_renderer_resources_instance.animations_instances[ zombie->hit_animation_index ].disabled && crude_entity_valid( it->world, game->player_node ) )
      {
        crude_physics_ray_cast_result                        ray_cast_result;
        XMMATRIX                                             player_to_world;
        XMMATRIX                                             ray_to_player_to_world;
        XMVECTOR                                             ray_direction, ray_origin;

        ray_to_player_to_world = crude_transform_node_to_world( it->world, crude_ecs_lookup_entity_from_parent( it->world, zombie_pivot_entity, "ray_player_start" ), NULL );
        
        player_to_world = crude_transform_node_to_world( it->world, crude_ecs_lookup_entity_from_parent( it->world, game->player_node, "character.health.player_collision" ), NULL );
        
        ray_direction = XMVectorScale( XMVectorSubtract( player_to_world.r[ 3 ], ray_to_player_to_world.r[ 3 ] ), 1.5 );
        ray_origin = ray_to_player_to_world.r[ 3 ];

        float32 angle = XMVectorGetX( XMVector3Dot( XMVector3Normalize( XMVectorSetY( ray_direction, 0 ) ), XMVector3Normalize( XMVectorSetY( XMVector3TransformNormal( XMVectorSet( 0, 0, 1, 0 ), ray_to_player_to_world ), 0 ) ) ) );
        if ( acos( angle ) < XM_PIDIV2 )
        {
          if ( crude_physics_ray_cast( &game->engine->physics, game->engine->world, ray_origin, ray_direction, g_crude_jph_broad_phase_layer_area_mask | g_crude_jph_broad_phase_layer_static_mask, g_crude_jph_mask_custom0 | g_crude_jph_mask_custom2, &ray_cast_result ) )
          {
            if ( ray_cast_result.layer & g_crude_jph_layer_custom2 )
            {
              XMStoreFloat3( &zombie->target_point, player_to_world.r[ 3 ] );
            }
          }
        }
      }

      if ( !zombie_model->model_renderer_resources_instance.animations_instances[ zombie->attack_animation_index ].disabled )
      {
        crude_gfx_model_renderer_resources_instance_blend_one_animation( &zombie_model->model_renderer_resources_instance, zombie->attack_animation_index );
      }
      else if ( !zombie_model->model_renderer_resources_instance.animations_instances[ zombie->hit_animation_index ].disabled )
      {
        crude_gfx_model_renderer_resources_instance_blend_one_animation( &zombie_model->model_renderer_resources_instance, zombie->hit_animation_index );
      }
      else if ( !zombie_model->model_renderer_resources_instance.animations_instances[ zombie->walk_animation_index ].disabled )
      {
        crude_gfx_model_renderer_resources_instance_blend_one_animation( &zombie_model->model_renderer_resources_instance, zombie->walk_animation_index );
      }
      else
      {
        crude_gfx_model_renderer_resources_instance_blend_one_animation( &zombie_model->model_renderer_resources_instance, zombie->idle_animation_index );
      }
    }
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_zombie_engine_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_zombie_engine_update_system_" );
  
  crude_game                                              *game;
  crude_zombie_system_context                             *ctx;
  crude_zombie                                            *zombie_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_zombie_system_context*, it->ctx );
  zombie_per_entity = ecs_field( it, crude_zombie, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input const                                     *input;
    crude_zombie                                          *zombie;
    crude_entity                                           zombie_entity;
    crude_entity                                           hitbox_entity;

    zombie_entity = crude_entity_from_iterator( it, i );

    zombie = &zombie_per_entity[ i ];
  }
  CRUDE_PROFILER_ZONE_END;
}

/**********************************************************
 *
 *                 API
 *
 *********************************************************/
void
crude_zombie_health_damage_callback_
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
)
{
  crude_game                                              *game;
  crude_zombie                                            *zombie;
  crude_gltf                                              *zombie_model;
  crude_entity                                             zombie_entity;
  crude_entity                                             zombie_pivot_entity;
  crude_entity                                             zombie_model_entity;

  game = crude_game_instance( );

  zombie_entity = crude_entity_get_parent( game->engine->world, health_entity );

  CRUDE_ASSERT( crude_entity_valid( game->engine->world, zombie_entity ) );

  zombie = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->engine->world, zombie_entity, crude_zombie );
  CRUDE_ASSERT( zombie );

  CRUDE_LOG_INFO( CRUDE_CHANNEL_GAMEPLAY, "Zombie %i got hit and get %i damage", zombie_entity, damage );
  
  zombie_pivot_entity = crude_ecs_lookup_entity_from_parent( game->engine->world, zombie_entity, "pivot" );
  zombie_model_entity = crude_ecs_lookup_entity_from_parent( game->engine->world, zombie_pivot_entity, "model" );

  zombie_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->engine->world, zombie_model_entity, crude_gltf );

  zombie_model->model_renderer_resources_instance.animations_instances[ zombie->hit_animation_index ].disabled = false;
  zombie_model->model_renderer_resources_instance.animations_instances[ zombie->hit_animation_index ].current_time = 0;

  zombie_model->model_renderer_resources_instance.animations_instances[ zombie->walk_animation_index ].disabled = false;
}

void
crude_zombie_health_death_callback_
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
)
{
  crude_game                                              *game;
  crude_zombie                                            *zombie;
  crude_gltf                                              *zombie_model;
  crude_entity                                             zombie_entity;
  crude_entity                                             zombie_pivot_entity;
  crude_entity                                             zombie_model_entity;

  game = crude_game_instance( );

  zombie_entity = crude_entity_get_parent( game->engine->world, health_entity );

  CRUDE_ASSERT( crude_entity_valid( game->engine->world, zombie_entity ) );

  zombie = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->engine->world, zombie_entity, crude_zombie );
  CRUDE_ASSERT( zombie );

  CRUDE_LOG_INFO( CRUDE_CHANNEL_GAMEPLAY, "Zombie %i got hit and get %i damage", zombie_entity, damage );
  
  zombie_pivot_entity = crude_ecs_lookup_entity_from_parent( game->engine->world, zombie_entity, "pivot" );
  zombie_model_entity = crude_ecs_lookup_entity_from_parent( game->engine->world, zombie_pivot_entity, "model" );

  zombie_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->engine->world, zombie_model_entity, crude_gltf );

  zombie_model->model_renderer_resources_instance.animations_instances[ zombie->dead_animation_index ].disabled = false;
  zombie_model->model_renderer_resources_instance.animations_instances[ zombie->dead_animation_index ].current_time = 0;

  zombie->dying = true;

  crude_entity_destroy_hierarchy( game->engine->world, health_entity );
}

void
crude_zombie_attack_callback_
(
  _In_ crude_entity                                        signal_entity,
  _In_ crude_entity                                        hitted_entity
)
{
  crude_game                                              *game;
  crude_zombie const                                      *zombie;
  crude_health                                            *health;
  crude_gltf                                              *zombie_model;
  crude_entity                                             health_entity;
  crude_entity                                             zombie_entity;
  crude_entity                                             zombie_pivot_entity;
  crude_entity                                             zombie_model_entity;

  game = crude_game_instance( );
  
  health_entity = hitted_entity;
  zombie_entity = signal_entity;

  health = CRUDE_ENTITY_FIND_COMPONENT_FROM_PARENTS( game->engine->world, &health_entity, crude_health );

  zombie = CRUDE_ENTITY_FIND_COMPONENT_FROM_PARENTS( game->engine->world, &zombie_entity, crude_zombie );
  
  if ( health )
  {
    crude_heal_receive_damage( health_entity, health, zombie->damage );
    
    zombie_pivot_entity = crude_ecs_lookup_entity_from_parent( game->engine->world, signal_entity, "pivot" );
    zombie_model_entity = crude_ecs_lookup_entity_from_parent( game->engine->world, zombie_pivot_entity, "model" );
    zombie_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->engine->world, zombie_model_entity, crude_gltf );

    zombie_model->model_renderer_resources_instance.animations_instances[ zombie->attack_animation_index ].disabled = false;
    zombie_model->model_renderer_resources_instance.animations_instances[ zombie->attack_animation_index ].current_time = 0;
  }
}