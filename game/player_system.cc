#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/scene_components.h>
#include <engine/platform/platform_components.h>
#include <engine/physics/physics_components.h>
#include <engine/physics/physics.h>
#include <engine/external/game_components.h>
#include <game/game.h>

#include <game/player_system.h>

#define CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE 30
#define CRUDE_GAME_PLAYER_DRUG_WITHDRAWAL_LIMIT 10

CRUDE_ECS_OBSERVER_DECLARE( crude_player_creation_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_player_update_system_ );

static void
crude_hitbox_callback
(
  _In_ void                                               *ctx
)
{
  static int b =0;
  b++;
}

static void
crude_player_update_visual_
(
  _In_ crude_player                                       *player
);

static void
crude_player_creation_observer_
(
  _In_ ecs_iter_t *it
)
{
  game_t *game = game_instance( );
  crude_player *player_per_entity = ecs_field( it, crude_player, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_player *player = &player_per_entity[ i ];
    crude_entity node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    crude_entity hitbox_node = crude_ecs_lookup_entity_from_parent( node, "hitbox" );
    crude_physics_character_body_handle *hitbox_body_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( hitbox_node, crude_physics_character_body_handle );
    crude_physics_character_body *hitbox_body = crude_physics_resources_manager_access_character_body( &game->physics_resources_manager, *hitbox_body_handle );
    hitbox_body->callback_container.fun = crude_hitbox_callback;

    player->health = 1.f;
    player->drug_withdrawal = 0.f;
    player->sanity = 1.f;
    player->stop_updating_gameplay_values = false;
  }
}

static void
crude_player_update_system_
(
  _In_ ecs_iter_t *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_player_update_system_" );
  game_t *game = game_instance( );
  crude_transform *transforms_per_entity = ecs_field( it, crude_transform, 0 );
  crude_player *player_per_entity = ecs_field( it, crude_player, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_player                                          *player;

    transform = &transforms_per_entity[ i ];
    player = &player_per_entity[ i ];
    if ( !player->stop_updating_gameplay_values )
    {
    }
    crude_player_update_visual_( player );
  }
  CRUDE_PROFILER_END;
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_player_system )
{
  ECS_MODULE( world, crude_player_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_game_components );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_player_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_player ) }
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_player_creation_observer_, EcsOnSet, NULL, { 
    { .id = ecs_id( crude_player ) }
  } );
}

void
crude_player_update_visual_
(
  _In_ crude_player                                       *player
)
{
  game_t                                                  *game;
  crude_gfx_game_postprocessing_pass_options              *pass_options;
  
  game = game_instance( );
  pass_options = &game->game_postprocessing_pass.options;

  /* Health Pulse Effect */
  pass_options->pulse_color = CRUDE_COMPOUNT( XMFLOAT4, { 1.f, 0.f, 0.f, 1.f } );
  pass_options->pulse_distance = 1.f;
  pass_options->pulse_distance_coeff  = 1.5f;
  pass_options->pulse_frame_scale = 0.02f;
  pass_options->pulse_scale = 1.f - player->health;
}