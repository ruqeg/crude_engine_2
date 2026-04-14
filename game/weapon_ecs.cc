#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/platform/platform.h>
#include <engine/graphics/imgui.h>
#include <game/game.h>

#include <game/weapon_ecs.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_weapon );
CRUDE_COMPONENT_STRING_DEFINE( crude_weapon, "crude_weapon" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_weapon )
{
  crude_memory_set( component, 0, sizeof( crude_weapon ) );
  component->max_ammo = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "max_ammo" ) );
  component->damage = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "damage" ) );
  component->max_cooldown = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "max_cooldown" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_weapon )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_weapon ) ) );
  cJSON_AddItemToObject( free_camera_json, "max_ammo", cJSON_CreateNumber( component->max_ammo ) );
  cJSON_AddItemToObject( free_camera_json, "damage", cJSON_CreateNumber( component->damage ) );
  cJSON_AddItemToObject( free_camera_json, "max_cooldown", cJSON_CreateNumber( component->max_cooldown ) );
  return free_camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_weapon )
{
  CRUDE_IMGUI_START_OPTIONS;

  CRUDE_IMGUI_OPTION( "Max ammo", {
    ImGui::DragInt( "##Max ammo", &component->max_ammo );
    });

  CRUDE_IMGUI_OPTION( "Damage", {
    ImGui::DragInt( "##Damage", &component->damage );
    });

  CRUDE_IMGUI_OPTION( "Max Cooldown", {
    ImGui::DragFloat( "##Max Cooldown", &component->max_cooldown );
    });
}

/**********************************************************
 *
 *                 API
 *
 *********************************************************/
bool
crude_weapon_fire
(
  _In_ crude_entity                                        weapon_entity,
  _In_ crude_weapon                                       *weapon
)
{
  crude_game                                             *game;
  crude_audio_player_handle                              *weapon_shot_audio_player_handle;
  XMVECTOR                                                direction, origin;
  XMMATRIX                                                weapon_to_world;
  crude_physics_ray_cast_result                           ray_cast_result;
  crude_entity                                            weapon_shot_audio_player_entity;
  
  game = crude_game_instance( );

  if ( weapon->wasted_ammo >= weapon->max_ammo )
  {
    return false;
  }

  if ( weapon->cooldown < weapon->max_cooldown )
  {
    return false;
  }

  weapon_shot_audio_player_entity = crude_ecs_lookup_entity_from_parent( game->engine->world, weapon_entity, "shot_audio_player" );
  weapon_shot_audio_player_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->engine->world, weapon_shot_audio_player_entity, crude_audio_player_handle );
  crude_audio_device_sound_start( &game->engine->audio_device, weapon_shot_audio_player_handle->sound_handle );

  weapon->cooldown = 0.f;

  weapon->wasted_ammo += 1;

  weapon_to_world = crude_transform_node_to_world( game->engine->world, weapon_entity, NULL );
  
  direction = XMVectorScale( XMVector3TransformNormal( XMVectorSet( 1, 0, 0, 0 ), weapon_to_world ), 10000 );
  origin = weapon_to_world.r[ 3 ];

  if ( crude_physics_ray_cast( &game->engine->physics, game->engine->world, origin, direction, g_crude_jph_broad_phase_layer_static_mask | g_crude_jph_broad_phase_layer_area_mask, g_crude_jph_mask_custom0 | g_crude_jph_mask_custom1, &ray_cast_result ) )
  {
    if ( ray_cast_result.layer & g_crude_jph_layer_custom1 )
    {
      crude_health                                        *health;
      crude_entity                                         entity;

      entity = ray_cast_result.entity;

      health = CRUDE_ENTITY_FIND_COMPONENT_FROM_PARENTS( game->engine->world, &entity, crude_health );

      if ( health )
      {
        crude_heal_receive_damage( entity, health, weapon->damage );
      }
    }
  }

  return true;
}

void
crude_weapon_give_ammo
(
  _In_ crude_weapon                                       *weapon,
  _In_ int32                                               ammo
)
{
  weapon->wasted_ammo -= ammo;
  if ( weapon->wasted_ammo < 0 )
  {
    weapon->wasted_ammo = 0;
  }
}

void
crude_weapon_fill_ammo
(
  _In_ crude_weapon                                       *weapon
)
{
  weapon->wasted_ammo = 0;
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_weapon_game_update_system_ );

void
crude_weapon_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_weapon_system_context                        *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_weapon_system );
  
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_weapon );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_weapon );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_weapon );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_weapon );

  crude_scene_components_import( world, manager );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_weapon_game_update_system_, crude_ecs_on_game_update, ctx, {
    { .id = ecs_id( crude_weapon ) },
  } );
}

void
crude_weapon_game_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_weapon_game_update_system_" );

  crude_game                                              *game;
  crude_weapon_system_context                             *ctx;
  crude_weapon                                            *weapon_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_weapon_system_context*, it->ctx );
  weapon_per_entity = ecs_field( it, crude_weapon, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_weapon                                          *weapon;

    weapon = &weapon_per_entity[ i ];

    weapon->cooldown += it->delta_time;
  }
  CRUDE_PROFILER_ZONE_END;
}