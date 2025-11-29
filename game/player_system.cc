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

#define CRUDE_GAME_PLAYER_SANITY_LIMIT ( 2.5 * 60 )
#define CRUDE_GAME_PLAYER_DRUG_WITHDRAWAL_LIMIT ( 5 * 60 )

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

CRUDE_API void
crude_player_update_values_
(
  _In_ crude_player                                       *player,
  _In_ float32                                             delta_time
);

CRUDE_API void
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
    player->stop_updating_visual_values = false;
  }
}

void
crude_player_update_system_
(
  _In_ ecs_iter_t                                         *it
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
      crude_player_update_values_( player, it->delta_time );
    }
    if ( !player->stop_updating_visual_values )
    {
      crude_player_update_visual_( player );
    }

    if ( player->health < 0.f || player->drug_withdrawal > 1.f || player->sanity < 0.f )
    {
      game_push_reload_scene_command( game );
    }
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
  
  /* Drug Effect */
  pass_options->wave_size = CRUDE_LERP( 0, 0.2, CRUDE_MAX( 0.f, player->drug_withdrawal - 0.2f ) );
  pass_options->wave_texcoord_scale = CRUDE_LERP( 0, 3.5, player->drug_withdrawal );
  pass_options->wave_absolute_frame_scale = CRUDE_LERP( 0.02, 0.035, player->drug_withdrawal );
  pass_options->aberration_strength_scale = CRUDE_LERP( 0.f, 0.1f, CRUDE_MAX( 0.f, player->drug_withdrawal - 0.5f ) );
  pass_options->aberration_strength_offset  = CRUDE_LERP( 0.0f, 0.05f, CRUDE_MAX( 0.f, player->drug_withdrawal - 0.5f ) );;
  pass_options->aberration_strength_sin_affect = CRUDE_LERP( 0.01f, 0.03f, player->drug_withdrawal );
  
  /* Sanity */
  pass_options->fog_color = CRUDE_COMPOUNT( XMFLOAT4, { 1.f, 0.f, 0.f, 0.f } );
  pass_options->fog_color.w = 5.f * CRUDE_MAX( 0.f, ( 0.5f - player->health ) );
  pass_options->fog_distance = CRUDE_LERP( 0.f, 25.f, player->sanity );

  /* Health Pulse Effect */
  pass_options->pulse_color = CRUDE_COMPOUNT( XMFLOAT4, { 1.f, 0.f, 0.f, 1.f + 3.f * ( 1.f - player->health ) } );
  pass_options->pulse_distance = CRUDE_LERP( 0.8f, 1.5f, player->health );
  pass_options->pulse_distance_coeff = CRUDE_LERP( 1.f, 3.0f, player->health );
  pass_options->pulse_frame_scale = CRUDE_LERP( 0.03f, 0.02f, player->health );
  pass_options->pulse_scale = 1.f - player->health;
}

void
crude_player_update_values_
(
  _In_ crude_player                                       *player,
  _In_ float32                                             delta_time
)
{
  player->drug_withdrawal += delta_time / CRUDE_GAME_PLAYER_DRUG_WITHDRAWAL_LIMIT;
  player->sanity -= delta_time / CRUDE_GAME_PLAYER_SANITY_LIMIT;
}