#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/scene_components.h>
#include <engine/platform/platform_components.h>
#include <engine/physics/physics_components.h>
#include <engine/physics/physics.h>
#include <engine/external/game_components.h>
#include <engine/audio/audio_components.h>
#include <game/recycle_station_system.h>
#include <game/game.h>
#include <game/enemy_system.h>
#include <game/teleport_station_system.h>

#include <game/player_system.h>

CRUDE_ECS_OBSERVER_DECLARE( crude_player_destroy_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_player_creation_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_player_update_system_ );

static void
crude_player_interaction_collision_callback
(
  _In_ void                                               *ctx,
  _In_ crude_entity                                        character_node,
  _In_ crude_entity                                        static_body_node,
  _In_ uint32                                              static_body_layer
)
{
  game_t *game = game_instance( );
  crude_input const *input;
  crude_player *player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player );
  crude_entity static_body_node_parent = crude_entity_get_parent( static_body_node );

  input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->platform_node, crude_input );

  if ( input->keys[ SDL_SCANCODE_E ].pressed && CRUDE_ENTITY_HAS_COMPONENT( game->main_node, crude_level_starting_room ) )
  {
    crude_level_starting_room *level = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_starting_room );
    
    for ( uint32 i = 0; i < CRUDE_GAME_PLAYER_ITEMS_MAX_COUNT; ++i )
    {
      if ( player->inventory_items[ i ] == CRUDE_GAME_ITEM_NONE )
      {
        if ( level->state == CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_HEALTH )
        {
          game_player_set_item( game, player, i, CRUDE_GAME_ITEM_SYRINGE_HEALTH );
        }
        else if ( level->state == CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_DRUG )
        {
          game_player_set_item( game, player, i, CRUDE_GAME_ITEM_SYRINGE_DRUG );
        }
        else if ( level->state == CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_CAN_MOVE )
        {
        }
        break;
      }
    }
    return;
  }

  if ( static_body_layer & 8 )
  {
    if ( input->keys[ SDL_SCANCODE_E ].pressed )
    {
      if ( crude_entity_valid( static_body_node_parent ) )
      {
        if ( CRUDE_ENTITY_HAS_COMPONENT( static_body_node_parent, crude_recycle_station ) )
        {
          crude_recycle_station *recycle_station = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( static_body_node_parent, crude_recycle_station );
          crude_recycle_station_start_recycle( recycle_station, player, static_body_node_parent );
        }
        else if ( CRUDE_ENTITY_HAS_COMPONENT( static_body_node_parent, crude_teleport_station ) )
        {
          crude_teleport_station *teleport_station = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( static_body_node_parent, crude_teleport_station );
          crude_teleport_station_set_serum( teleport_station, player, static_body_node_parent );
        }
      }

      if ( CRUDE_ENTITY_HAS_TAG( static_body_node, crude_serum_station_enabled ) )
      {
        for ( uint32 i = 0; i < CRUDE_GAME_PLAYER_ITEMS_MAX_COUNT; ++i )
        {
          if ( player->inventory_items[ i ] == CRUDE_GAME_ITEM_NONE )
          {
            crude_level_01 *level = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_01 );
            crude_audio_device_sound_set_translation( &game->audio_device, level->take_serum_sound_handle, crude_transform_node_to_world( static_body_node, CRUDE_ENTITY_GET_MUTABLE_COMPONENT( static_body_node, crude_transform ) ).r[ 3 ] );
            crude_audio_device_sound_start( &game->audio_device, level->take_serum_sound_handle );
            game_player_set_item( game, player, i, CRUDE_GAME_ITEM_SERUM );
            CRUDE_ENTITY_REMOVE_TAG( static_body_node, crude_serum_station_enabled );
            game_push_enable_random_serum_station_command( game, static_body_node );
            break;
          }
        }
      }
    }
  }
  else if ( static_body_layer & ( 1 << 7 ) )
  {
    player->inside_safe_zone = true;
  }
  else if ( static_body_layer & ( 1 << 9 ) )
  {
    crude_level_starting_room const *level = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->main_node, crude_level_starting_room );
    if ( input->keys[ SDL_SCANCODE_E ].current && level->state == CRUDE_LEVEL_STARTING_ROOM_STATE_NEED_CAN_MOVE )
    {
      game_push_load_scene_command( game, game->level_cutscene0_node_absolute_filepath );
    }
  }
}

static void
crude_player_enemy_hitbox_callback
(
  _In_ void                                               *ctx,
  _In_ crude_entity                                        character_node,
  _In_ crude_entity                                        static_body_node,
  _In_ uint32                                              static_body_layer
)
{
  game_t *game = game_instance( );
  
  if ( CRUDE_ENTITY_HAS_COMPONENT( game->main_node, crude_level_boss_fight ) )
  {
    crude_entity_destroy_hierarchy( crude_entity_get_parent( static_body_node ) );
    crude_player *player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player );
    player->health -= 0.25;
  }
  else
  {
    crude_enemy *enemy = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( crude_entity_get_parent( crude_entity_get_parent( static_body_node ) ), crude_enemy );
    crude_player *player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player );
    crude_transform const *player_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->player_node, crude_transform );
    
    crude_enemy_deal_damage_to_player( enemy, player, XMLoadFloat3( &player_transform->translation ) );
  }
}

static void
crude_player_hit_callback
(
  _In_ void                                               *ctx,
  _In_ crude_entity                                        character_node,
  _In_ crude_entity                                        static_body_node,
  _In_ uint32                                              static_body_layer
)
{
  game_t *game = game_instance( );
  
  crude_player *player = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player );
  crude_input const *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->platform_node, crude_input );
  crude_level_starting_room *level = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_starting_room );
  
  static uint32 sound_count = 0;

  if ( input->mouse.left.pressed && !crude_audio_device_sound_is_playing( &game->audio_device, level->hit_0_sound_handle ) && !crude_audio_device_sound_is_playing( &game->audio_device, level->hit_1_sound_handle ) && !crude_audio_device_sound_is_playing( &game->audio_device, level->hit_2_sound_handle ) )
  {
    player->sanity += 0.2;
    if ( sound_count == 0 )
    {
      crude_audio_device_sound_start( &game->audio_device, level->hit_0_sound_handle );
    }
    else if ( sound_count == 1 )
    {
      crude_audio_device_sound_start( &game->audio_device, level->hit_1_sound_handle );
    }
    else if ( sound_count == 2 )
    {
      crude_audio_device_sound_start( &game->audio_device, level->hit_2_sound_handle );
    }
    sound_count = ( sound_count + 1 ) % 3;
  }
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
    CRUDE_ENTITY_SET_COMPONENT( crude_ecs_lookup_entity_from_parent( node, "pivot" ), crude_audio_listener, {} );

    crude_entity hitbox_node = crude_ecs_lookup_entity_from_parent( node, "enemy_hitbox" );
    crude_physics_character_body_handle *hitbox_body_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( hitbox_node, crude_physics_character_body_handle );
    crude_physics_character_body *hitbox_body = crude_physics_resources_manager_access_character_body( &game->physics_resources_manager, *hitbox_body_handle );
    
    hitbox_body->callback_container.ctx = (void*)1;
    hitbox_body->callback_container.fun = crude_player_enemy_hitbox_callback;
    
    crude_entity interaction_collision_node = crude_ecs_lookup_entity_from_parent( node, "interaction_collision" );
    crude_physics_character_body_handle *interaction_collision_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( interaction_collision_node, crude_physics_character_body_handle );
    crude_physics_character_body *interaction_collision_body = crude_physics_resources_manager_access_character_body( &game->physics_resources_manager, *interaction_collision_handle );
    
    interaction_collision_body->callback_container.ctx = (void*)1;
    interaction_collision_body->callback_container.fun = crude_player_interaction_collision_callback;
    
    crude_entity mannequin_interaction_node = crude_ecs_lookup_entity_from_parent( node, "mannequin_interaction" );
    if ( crude_entity_valid( mannequin_interaction_node ) )
    {
      crude_physics_character_body_handle *mannequin_interaction_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( mannequin_interaction_node, crude_physics_character_body_handle );
      crude_physics_character_body *mannequin_interaction_body = crude_physics_resources_manager_access_character_body( &game->physics_resources_manager, *mannequin_interaction_handle );
      
      mannequin_interaction_body->callback_container.ctx = (void*)1;
      mannequin_interaction_body->callback_container.fun = crude_player_hit_callback;
    }

    player->health = 1.f;
    player->drug_withdrawal = 0.f;
    player->sanity = 1.f;
    player->stop_updating_gameplay_values = false;
    player->stop_updating_visual_values = false;
    player->inventory_items[ 0 ] = CRUDE_GAME_ITEM_NONE;
    player->inventory_items[ 1 ] = CRUDE_GAME_ITEM_NONE;
    player->inventory_items[ 2 ] = CRUDE_GAME_ITEM_NONE;

    crude_sound_creation sound_creation = crude_sound_creation_empty( );
    sound_creation.async_loading = true;
    sound_creation.absolute_filepath = game->walking_sound_absolute_filepath;
    sound_creation.looping = true;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    player->walking_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );

    sound_creation = crude_sound_creation_empty( );
    sound_creation.async_loading = true;
    sound_creation.absolute_filepath = game->syringe_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    player->syringe_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );

    sound_creation = crude_sound_creation_empty( );
    sound_creation.async_loading = true;
    sound_creation.absolute_filepath = game->reload_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    player->reload_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );

    sound_creation = crude_sound_creation_empty( );
    sound_creation.looping = true;
    sound_creation.stream = true;
    sound_creation.absolute_filepath = game->heartbeat_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    player->heartbeat_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
    crude_audio_device_sound_start( &game->audio_device, player->heartbeat_sound_handle );
    crude_audio_device_sound_set_volume( &game->audio_device, player->heartbeat_sound_handle, 0.f );

    sound_creation = crude_sound_creation_empty( );
    sound_creation.looping = true;
    sound_creation.stream = true;
    sound_creation.decode = true;
    sound_creation.absolute_filepath = game->ghost_sound_absolute_filepath;
    sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
    player->ghost_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
    crude_audio_device_sound_start( &game->audio_device, player->ghost_sound_handle );
    crude_audio_device_sound_set_volume( &game->audio_device, player->ghost_sound_handle, 0.f );
    
    crude_audio_device_sound_start( &game->audio_device, player->walking_sound_handle );
    crude_audio_device_sound_set_volume( &game->audio_device, player->walking_sound_handle, 0.f );
  }
}

static void
crude_player_destroy_observer_
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

    crude_audio_device_destroy_sound( &game->audio_device, player->walking_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, player->syringe_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, player->reload_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, player->heartbeat_sound_handle );
    crude_audio_device_destroy_sound( &game->audio_device, player->ghost_sound_handle );
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

  crude_input const *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->platform_node, crude_input );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_player                                          *player;

    transform = &transforms_per_entity[ i ];
    player = &player_per_entity[ i ];

    if ( !player->inside_safe_zone && !player->stop_updating_gameplay_values )
    {
      crude_player_update_values_( player, it->delta_time );
    }
    if ( !player->stop_updating_visual_values )
    {
      crude_player_update_visual_( player );
    }

    
    if ( game->death_screen )
    {
      crude_audio_device_sound_set_volume( &game->audio_device, game->death_sound_handle, CRUDE_LERP( 1.5, 0, CRUDE_MAX( 0, player->time_to_reload_scene - 2 ) ) );
    }
    else
    {
      crude_audio_device_sound_set_volume( &game->audio_device, player->ghost_sound_handle, 2 * pow( ( player->sanity > 0.5f ? 0.f : ( 1 -  2 * player->sanity ) ), 3.f ) );
      crude_audio_device_sound_set_volume( &game->audio_device, player->heartbeat_sound_handle, 3 * ( player->health > 0.5f ? 0.f : ( 1 -  2 * player->health ) ) );
      if ( player->inside_safe_zone )
      {
        player->safe_zone_volume = CRUDE_MIN( 1.05f, player->safe_zone_volume + 0.2 * it->delta_time );
      }
      else
      {
        player->safe_zone_volume = CRUDE_MAX( -0.05f, player->safe_zone_volume - 0.2 * it->delta_time );
      }

      if ( player->safe_zone_volume > 0 && player->safe_zone_volume < 1.0 )
      {
        crude_level_01 *level = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_01 );

        if ( level )
        {
          crude_audio_device_sound_set_volume( &game->audio_device, level->save_theme_sound_handle, CRUDE_MAX( 0.f, CRUDE_MIN( player->safe_zone_volume, 1.f ) ) );
          crude_audio_device_sound_set_volume( &game->audio_device, level->ambient_sound_handle, CRUDE_MAX( 0.f, CRUDE_MIN( 1.f, 1.f - player->safe_zone_volume ) ) );
        }
      }
    }

    player->inside_safe_zone = false;

    if ( !game->death_screen && ( player->health < 0.f || player->drug_withdrawal > 1.f || player->sanity < 0.f ) )
    {
      crude_player_controller *player_controller = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->player_node, crude_player_controller );
      player_controller->input_enabled = false;

      game->death_screen = true;
      if ( player->health < 0.f )
      {
        game->death_reason = "Bleeding";
      }
      else if ( player->drug_withdrawal > 1.f )
      {
        game->death_reason = "Drug Withdrawal";
      }
      else if ( player->sanity < 0.f )
      {
        game->death_reason = "Cognitive Distortion";
      }
      player->time_to_reload_scene = 3.f;
      crude_audio_device_sound_reset( &game->audio_device, game->death_sound_handle );
      crude_audio_device_sound_start( &game->audio_device, game->death_sound_handle );
      crude_level_01 *level = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_01 );
      crude_level_boss_fight *level_boss = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( game->main_node, crude_level_boss_fight );

      if ( level )
      {
        crude_audio_device_sound_set_volume( &game->audio_device, level->save_theme_sound_handle, 0 );
        crude_audio_device_sound_set_volume( &game->audio_device, level->ambient_sound_handle, 0 );
      }
      if ( level_boss )
      {
        crude_audio_device_sound_set_volume( &game->audio_device, level_boss->background_sound_handle, 0 );
      }
      crude_audio_device_sound_set_volume( &game->audio_device, player->heartbeat_sound_handle, 0 );
      crude_audio_device_sound_set_volume( &game->audio_device, player->ghost_sound_handle, 0 );
    }

    if ( game->death_screen && player->time_to_reload_scene < 0 && input->keys[ SDL_SCANCODE_SPACE ].current )
    {
      game_push_reload_scene_command( game );
      crude_audio_device_sound_stop( &game->audio_device, game->death_sound_handle );
    }

    player->time_to_reload_scene -= it->delta_time;

    {
      crude_entity player_items_node = crude_ecs_lookup_entity_from_parent( game->player_node, "pivot.items" );
      crude_transform *player_items_trasnform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( player_items_node, crude_transform );
      player_items_trasnform->translation.y = 0.004f * sin( game->time );
    }
  }
  CRUDE_PROFILER_ZONE_END;
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
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_player_destroy_observer_, EcsOnRemove, NULL, { 
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
  
  if ( !CRUDE_ENTITY_HAS_COMPONENT( game->main_node, crude_level_boss_fight ) )
  {
    /* Drug Effect */
    pass_options->wave_size = CRUDE_LERP( 0, 0.1, pow( player->drug_withdrawal, 4.f ) );
    pass_options->wave_texcoord_scale = CRUDE_LERP( 0, 3.f, pow( player->drug_withdrawal, 4.f ) );
    pass_options->wave_absolute_frame_scale = 100 * CRUDE_LERP( 0.02, 0.025, player->drug_withdrawal );
    pass_options->aberration_strength_scale = CRUDE_LERP( 0.f, 0.046f, pow( player->drug_withdrawal, 6.f ) );
    pass_options->aberration_strength_offset  = CRUDE_LERP( 0.0f, 0.015f, pow( player->drug_withdrawal, 6.f ) );;
    pass_options->aberration_strength_sin_affect = CRUDE_LERP( 0.01f, 0.02f, pow( player->drug_withdrawal, 2.f ) );
    
    /* Sanity */
    pass_options->fog_color = CRUDE_COMPOUNT_EMPTY( XMFLOAT4 );
    pass_options->fog_color.w = 0.f;
    pass_options->fog_distance = CRUDE_LERP( 0.f, CRUDE_GAME_PLAYER_MAX_FOG_DISTANCE, pow( player->sanity, 0.75 ) );
  }
  /* Health Pulse Effect */
  pass_options->pulse_color = CRUDE_COMPOUNT( XMFLOAT4, { 1.f, 0.f, 0.f, 0.f } );
  pass_options->pulse_color.w = CRUDE_LERP( 4.f, 0.5f, pow( player->health, 0.25f ) );
  pass_options->pulse_distance = CRUDE_LERP( 0.8f, 4.f, pow( player->health, 0.5f ) );
  pass_options->pulse_distance_coeff = 1.5f;
  pass_options->pulse_frame_scale = 100 * CRUDE_LERP( 0.03f, 0.005f, pow( player->health, 0.25f ) );
  pass_options->pulse_scale = 1.f - player->health;

  /* Death Screen */
  game->death_overlap_color.w = game->death_screen ? CRUDE_LERP( 1, 0, CRUDE_MAX( 0, player->time_to_reload_scene - 2 ) ) : 0.f;
}

void
crude_player_update_values_
(
  _In_ crude_player                                       *player,
  _In_ float32                                             delta_time
)
{
  
  game_t *game = game_instance( );
  if ( !CRUDE_ENTITY_HAS_COMPONENT( game->main_node, crude_level_boss_fight ) )
  {
    player->drug_withdrawal += delta_time / CRUDE_GAME_PLAYER_DRUG_WITHDRAWAL_LIMIT;
    player->sanity -= delta_time / CRUDE_GAME_PLAYER_SANITY_LIMIT;
  }
}