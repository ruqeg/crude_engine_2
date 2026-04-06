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
ECS_COMPONENT_DECLARE( crude_zombie );
CRUDE_COMPONENT_STRING_DEFINE( crude_zombie, "crude_zombie" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_zombie )
{
  crude_memory_set( component, 0, sizeof( crude_zombie ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_zombie )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_zombie ) ) );
  return free_camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_zombie )
{
  CRUDE_IMGUI_START_OPTIONS;
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_OBSERVER_DECLARE( crude_zombie_create_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_zombie_update_system_ );

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
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_zombie );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_zombie );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_zombie );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_zombie_update_system_, crude_ecs_on_game_update, ctx, {
    { .id = ecs_id( crude_zombie ) },
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_zombie_create_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_zombie ), .oper = EcsAnd }
  } );

  crude_scene_components_import( world, manager );
}

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
    crude_gltf                                            *zombie_model;
    crude_entity                                           zombie_pivot_entity;
    crude_entity                                           zombie_model_entity;
    crude_entity                                           zombie_entity;

    zombie_entity = crude_entity_from_iterator( it, i );

    zombie = &zombie_per_entity[ i ];
    
    zombie_pivot_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_entity, "pivot" );
    zombie_model_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_pivot_entity, "model" );

    zombie_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, zombie_model_entity, crude_gltf );
    
    zombie->idle_animation_index = 0;

    zombie_model->model_renderer_resources_instance.animations_instances[ zombie->idle_animation_index ].animation_index = crude_gfx_model_renderer_resources_instance_find_animation_index_by_name(
      &zombie_model->model_renderer_resources_instance, &game->engine->model_renderer_resources_manager, "crazy" );

    zombie_model->model_renderer_resources_instance.animations_instances[ zombie->idle_animation_index ].disabled = false;
  }
  CRUDE_PROFILER_ZONE_END;
}


void
crude_zombie_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_zombie_update_system_" );
  
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
    crude_gltf                                            *zombie_model;
    crude_entity                                           zombie_pivot_entity;
    crude_entity                                           zombie_model_entity;
    crude_entity                                           zombie_entity;

    zombie_entity = crude_entity_from_iterator( it, i );

    zombie = &zombie_per_entity[ i ];
    
    zombie_pivot_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_entity, "pivot" );
    zombie_model_entity = crude_ecs_lookup_entity_from_parent( it->world, zombie_pivot_entity, "model" );

    zombie_model = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( it->world, zombie_model_entity, crude_gltf );
    
    crude_gfx_model_renderer_resources_instance_blend_one_animation( &zombie_model->model_renderer_resources_instance, zombie->idle_animation_index );
  }
  CRUDE_PROFILER_ZONE_END;
}