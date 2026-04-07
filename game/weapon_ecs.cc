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
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_weapon )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_weapon ) ) );
  return free_camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_weapon )
{
  CRUDE_IMGUI_START_OPTIONS;
}

/**********************************************************
 *
 *                 API
 *
 *********************************************************/
void
crude_weapon_fire
(
  _In_ crude_weapon                                       *weapon
)
{
  crude_game                                             *game;
  XMVECTOR                                                direction, origin;
  XMMATRIX                                                view_to_world;
  crude_physics_ray_cast_result                           ray_cast_result;
  
  game = crude_game_instance( );

  if ( weapon->ammo < 0 )
  {
    return;
  }

  view_to_world = crude_transform_node_to_world( game->engine->world, game->engine->camera_node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->engine->world, game->engine->camera_node, crude_transform ) );
  
  direction = XMVectorScale( XMVector3TransformNormal( XMVectorSet( 0, 0, -1, 0 ), view_to_world ), 10000000 );
  origin = view_to_world.r[ 3 ];

  weapon->ammo -= 1;

  if ( crude_physics_ray_cast( &game->engine->physics, game->engine->world, origin, direction, g_crude_jph_layer_non_moving, &ray_cast_result ) )
  {
    crude_health                                          *health;
    crude_entity                                           entity;

    entity = ray_cast_result.entity;
    while ( entity = crude_entity_get_parent( game->engine->world, entity ) )
    {
      if ( !crude_entity_valid( game->engine->world, entity ) )
      {
        break;
      }

      health = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->engine->world, entity, crude_health );

      if ( health )
      {
        crude_heal_receive_damage( health, weapon->damage );
      }
    }
  }
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_weapon_update_system_ );

void
crude_weapon_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_weapon_update_system_" );

  crude_game                                              *game;
  crude_weapon_system_context                             *ctx;
  crude_weapon                                            *weapon_per_entity;

  game = crude_game_instance( );
  ctx = CRUDE_CAST( crude_weapon_system_context*, it->ctx );
  weapon_per_entity = ecs_field( it, crude_weapon, 0 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
  }
  CRUDE_PROFILER_ZONE_END;
}

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
}