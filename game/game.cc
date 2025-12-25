#include <engine/core/profiler.h>
#include <engine/core/hash_map.h>
#include <engine/core/file.h>
#include <engine/core/memory.h>
#include <engine/core/process.h>
#include <engine/scene/scene_components.h>
#include <engine/scene/scripts_components.h>
#include <engine/graphics/gpu_resources_loader.h>
#include <engine/graphics/imgui.h>
#include <engine/physics/physics.h>
#include <engine/physics/physics_debug_system.h>
#include <engine/physics/physics_system.h>
#include <engine/external/game_components.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/free_camera_system.h>
#include <engine/core/time.h>
#include <game/player_controller_system.h>
#include <game/player_system.h>
#include <game/enemy_system.h>
#include <game/level_01_system.h>
#include <game/serum_station_system.h>
#include <game/recycle_station_system.h>
#include <game/weapon_system.h>
#include <game/level_starting_room_system.h>
#include <game/level_cutscene_only_sound_system.h>
#include <game/level_boss_fight_system.h>
#include <game/boss_system.h>

#include <game/game.h>

game_t                                                    *game_instance_;

CRUDE_ECS_SYSTEM_DECLARE( game_update_system_ );
CRUDE_ECS_SYSTEM_DECLARE( game_graphics_system_ );
CRUDE_ECS_SYSTEM_DECLARE( game_input_system_ );


static void
game_initialize_constant_strings_
(
  _In_ game_t                                             *game,
  _In_ char const                                         *scene_relative_filepath,
  _In_ char const                                         *render_graph_relative_directory,
  _In_ char const                                         *resources_relative_directory,
  _In_ char const                                         *shaders_relative_directory,
  _In_ char const                                         *techniques_relative_directory,
  _In_ char const                                         *compiled_shaders_relative_directory,
  _In_ char const                                         *working_absolute_directory
);

static void
game_deinitialize_constant_strings_
(
  _In_ game_t                                             *game
);


static void
game_initialize_audio_
(
  _In_ game_t                                             *game
);

static void
game_deinitialize_audio_
(
  _In_ game_t                                             *game
);

static void
game_initialize_graphics_
(
  _In_ game_t                                             *game
);

static void
game_graphics_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
game_update_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
game_input_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
game_setup_custom_preload_nodes_
(
  _In_ game_t                                             *game
);

static void
game_setup_custom_postload_nodes_
(
  _In_ game_t                                             *game
);

static void
game_setup_custom_postload_model_resources_
(
  _In_ game_t                                             *game
);

#include <miniaudio.h>
void
game_initialize
(
  _In_ game_t                                             *game,
  _In_ crude_game_creation const                          *creation
)
{
//  game->engine = creation->engine;
//  game->framerate = creation->framerate;
//  game->last_graphics_update_time = 0.f;
//  game->time = 0.f;
//  game->sensetivity = 1.f;
//  game->death_screen = false;
//  game->focused_camera_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
//
//  ECS_IMPORT( game->engine->world, crude_platform_system );
//  ECS_IMPORT( game->engine->world, crude_player_controller_system );
//  ECS_IMPORT( game->engine->world, crude_enemy_system );
//  ECS_IMPORT( game->engine->world, crude_level_01_system );
//  ECS_IMPORT( game->engine->world, crude_player_system );
//  ECS_IMPORT( game->engine->world, crude_serum_station_system );
//  ECS_IMPORT( game->engine->world, crude_weapon_system );
//  ECS_IMPORT( game->engine->world, crude_recycle_station_system );
//  ECS_IMPORT( game->engine->world, crude_level_starting_room_system );
//  ECS_IMPORT( game->engine->world, crude_level_cutscene_only_sound_system );
//  ECS_IMPORT( game->engine->world, crude_level_boss_fight_system );
//  ECS_IMPORT( game->engine->world, crude_boss_system );
  ECS_IMPORT( game->engine->world, crude_free_camera_system );
//  
//  srand( time( NULL ) );
//
//  game_initialize_constant_strings_( game, creation->scene_relative_filepath, creation->render_graph_relative_directory, creation->resources_relative_directory, creation->shaders_relative_directory, creation->techniques_relative_directory, creation->compiled_shaders_relative_directory, creation->working_absolute_directory );
//  
//#if CRUDE_DEVELOP
//  game->game_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_game_debug_system_context );
//  game->game_debug_system_context.enemy_spawnpoint_model_absolute_filepath = game->enemy_spawnpoint_debug_model_absolute_filepath;
//  game->game_debug_system_context.syringe_serum_station_active_model_absolute_filepath = game->syringe_serum_station_active_debug_model_absolute_filepath;
//  game->game_debug_system_context.syringe_spawnpoint_model_absolute_filepath = game->syringe_spawnpoint_debug_model_absolute_filepath;
//  crude_game_debug_system_import( game->engine->world, &game->game_debug_system_context );
//#endif
//
//  game_initialize_audio_( game );
//  game_initialize_graphics_( game );
//  
//#if CRUDE_DEVELOP
//  crude_devmenu_initialize( &game->devmenu );
//#endif
//  crude_game_menu_initialize( &game->game_menu );
//  
//  crude_audio_device_wait_wait_till_uploaded( &game->audio_device );
//
//  crude_physics_enable_simulation( &game->physics, true );
//
//  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_graphics_system_, EcsPreStore, game, { } );
//  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_update_system_, EcsPreStore, game, { } );
//  
//  CRUDE_ECS_SYSTEM_DEFINE( game->engine->world, game_input_system_, EcsOnUpdate, game, {
//    { .id = ecs_id( crude_input ) },
//    { .id = ecs_id( crude_window_handle ) },
//  } );
//
//  game->engine->last_update_time = crude_time_now( );
}

void
game_postupdate
(
  _In_ game_t                                             *game
)
{
  CRUDE_PROFILER_ZONE_NAME( "game_postupdate" );
#if CRUDE_DEVELOP
  crude_devmenu_update( &game->devmenu );
#endif
  crude_game_menu_update( &game->game_menu );
  
  CRUDE_PROFILER_ZONE_END( "game_postupdate" );
}

void
game_deinitialize
(
  _In_ game_t                                             *game
)
{
#if CRUDE_DEVELOP
  crude_devmenu_deinitialize( &game->devmenu );
#endif
  crude_game_menu_deinitialize( &game->game_menu );
  game_deinitialize_audio_( game );
  game_deinitialize_constant_strings_( game );
}

void
game_player_set_item
(
  _In_ game_t                                             *game,
  _In_ crude_player                                       *player,
  _In_ uint32                                              slot,
  _In_ crude_game_item                                     item
)
{
  crude_entity                                             player_items_node, player_item_node;
  char                                                     item_node_name_buffer[ 128 ];;

  player_items_node = crude_ecs_lookup_entity_from_parent( game->player_node, "pivot.items" );
  
  crude_snprintf( item_node_name_buffer, sizeof( item_node_name_buffer ), "item_%i", slot );
  player_item_node = crude_ecs_lookup_entity_from_parent( player_items_node, item_node_name_buffer );
  
  player->inventory_items[ slot ] = item;

  switch ( item )
  {
  case CRUDE_GAME_ITEM_NONE:
  {
    CRUDE_ENTITY_REMOVE_COMPONENT( player_item_node, crude_gltf );
    break;
  }
  case CRUDE_GAME_ITEM_SERUM:
  {
    CRUDE_ENTITY_SET_COMPONENT( player_item_node, crude_gltf, { game->serum_model_absolute_filepath } );
    break;
  }
  case CRUDE_GAME_ITEM_SYRINGE_DRUG:
  {
    if ( CRUDE_ENTITY_HAS_COMPONENT( game->main_node, crude_level_01 ) )
    {
      CRUDE_ENTITY_SET_COMPONENT( player_item_node, crude_gltf, { game->syringe_drug_model_absolute_filepath } );
    }
    else
    {
      CRUDE_ENTITY_SET_COMPONENT( player_item_node, crude_gltf, { game->starting_room_modern_syringe_drug_model_absolute_filepath } );
    }
    break;
  }
  case CRUDE_GAME_ITEM_SYRINGE_HEALTH:
  {
    if ( CRUDE_ENTITY_HAS_COMPONENT( game->main_node, crude_level_01 ) )
    {
      CRUDE_ENTITY_SET_COMPONENT( player_item_node, crude_gltf, { game->syringe_health_model_absolute_filepath } );
    }
    else
    {
      CRUDE_ENTITY_SET_COMPONENT( player_item_node, crude_gltf, { game->starting_room_modern_syringe_health_model_absolute_filepath } );
    }
    break;
  }
  case CRUDE_GAME_ITEM_AMMUNITION:
  {
    CRUDE_ENTITY_SET_COMPONENT( player_item_node, crude_gltf, { game->ammo_box_model_absolute_filepath } );
    break;
  }
  }
  CRUDE_ENTITY_ADD_COMPONENT( player_item_node, crude_node_runtime );
}

void
game_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "game_update_system_" );
  game_t                                                  *game;

  game = ( game_t* )it->ctx;
  game->time += it->delta_time;
  CRUDE_PROFILER_ZONE_END;
}

void
game_graphics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
}

void
game_setup_custom_preload_nodes_
(
  _In_ game_t                                             *game
)
{
//  char const *enemy_node_relative_filepath = "game\\nodes\\enemy.crude_node";
//  char const *serum_station_node_relative_filepath = "game\\nodes\\serum_station.crude_node";
//  char const *boss_bullet_node_relative_filepath = "game\\nodes\\boss_bullet.crude_node";
//
//  game->enemy_node_absolute_filepath = crude_string_buffer_append_use_f( &game->game_strings_buffer, "%s%s", game->resources_absolute_directory, enemy_node_relative_filepath );
//  game->template_enemy_node = crude_node_manager_get_node( &game->node_manager, game->enemy_node_absolute_filepath );
//  crude_entity_enable_hierarchy( game->template_enemy_node, false );
//
//  game->serum_station_node_absolute_filepath = crude_string_buffer_append_use_f( &game->game_strings_buffer, "%s%s", game->resources_absolute_directory, serum_station_node_relative_filepath );
//  game->template_serum_station_node = crude_node_manager_get_node( &game->node_manager, game->serum_station_node_absolute_filepath );
//  crude_entity_enable_hierarchy( game->template_serum_station_node, false );
//
//  game->boss_bullet_node_absolute_filepath = crude_string_buffer_append_use_f( &game->game_strings_buffer, "%s%s", game->resources_absolute_directory, boss_bullet_node_relative_filepath );
//  game->template_boss_bullet_node = crude_node_manager_get_node( &game->node_manager, game->boss_bullet_node_absolute_filepath );
//  crude_entity_enable_hierarchy( game->template_boss_bullet_node, false );
//
//  game->focused_camera_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
}

void
game_setup_custom_postload_nodes_
(
  _In_ game_t                                             *game
)
{
  game->player_node = crude_ecs_lookup_entity_from_parent( game->main_node, "player" );

  /*
    crude_entity editor_camera_node;
  {
    crude_transform                                        editor_camera_node_transform;
    crude_camera                                           editor_camera_node_camera;
    crude_free_camera                                      editor_camera_node_crude_free_camera;


    editor_camera_node_transform = CRUDE_COMPOUNT_EMPTY( crude_transform );
    XMStoreFloat4( &editor_camera_node_transform.rotation, XMQuaternionIdentity( ) );
    XMStoreFloat3( &editor_camera_node_transform.scale, XMVectorSplatOne( ) );
    XMStoreFloat3( &editor_camera_node_transform.translation, XMVectorZero( ) );

    editor_camera_node_camera = CRUDE_COMPOUNT_EMPTY( crude_camera );
    editor_camera_node_camera.fov_radians = 1;
    editor_camera_node_camera.aspect_ratio = 1.8;
    editor_camera_node_camera.near_z = 0.001;
    editor_camera_node_camera.far_z = 1000;

    editor_camera_node_crude_free_camera = CRUDE_COMPOUNT_EMPTY( crude_free_camera );
    editor_camera_node_crude_free_camera.moving_speed_multiplier = 10.f;
    editor_camera_node_crude_free_camera.rotating_speed_multiplier = -0.004f;
    editor_camera_node_crude_free_camera.input_enabled = true;
    editor_camera_node_crude_free_camera.input_node = game->platform_node;

    editor_camera_node = crude_entity_create_empty( game->engine->world, "editor_camera" );
    CRUDE_ENTITY_SET_COMPONENT( editor_camera_node, crude_transform, { editor_camera_node_transform } );
    CRUDE_ENTITY_SET_COMPONENT( editor_camera_node, crude_camera, { editor_camera_node_camera } );
    CRUDE_ENTITY_SET_COMPONENT( editor_camera_node, crude_free_camera, { editor_camera_node_crude_free_camera } );
  }

  game->focused_camera_node = editor_camera_node;*/
}

void
game_setup_custom_postload_model_resources_
(
  _In_ game_t                                             *game
)
{
  // small hack
//  crude_gfx_model_renderer_resources_manager_get_gltf_model( &game->model_renderer_resources_manager, game->serum_model_absolute_filepath , NULL );
}

void
game_input_system_
(
  _In_ ecs_iter_t                                         *it
)
{
//  CRUDE_PROFILER_ZONE_NAME( "game_input_system_" );
//  game_t *game = ( game_t* )it->ctx;
//  crude_input *input_per_entity = ecs_field( it, crude_input, 0 );
//  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );
//  
//  ImGui::SetCurrentContext( CRUDE_CAST( ImGuiContext*, game->imgui_context ) );
//  for ( uint32 i = 0; i < it->count; ++i )
//  {
//    crude_input *input = &input_per_entity[ i ];
//    crude_window_handle *window_handle = &window_handle_per_entity[ i ];
//#if CRUDE_DEVELOP
//    crude_devmenu_handle_input( &game->devmenu, input );
//#endif
//    crude_game_menu_handle_input( &game->game_menu, input );
//  }
//  CRUDE_PROFILER_ZONE_END;
}

void
game_initialize_constant_strings_
(
  _In_ game_t                                             *game,
  _In_ char const                                         *scene_relative_filepath,
  _In_ char const                                         *render_graph_relative_directory,
  _In_ char const                                         *resources_relative_directory,
  _In_ char const                                         *shaders_relative_directory,
  _In_ char const                                         *techniques_relative_directory,
  _In_ char const                                         *compiled_shaders_relative_directory,
  _In_ char const                                         *working_absolute_directory
)
{
  char const *serum_model_relative_filepath = "game\\models\\serum.gltf";
  char const *boss_bullet_model_relative_filepath = "game\\models\\boss_bullet.gltf";
  char const *syringe_drug_model_relative_filepath = "game\\models\\Syringe.gltf";
  char const *syringe_health_model_relative_filepath = "game\\models\\Syringe_Health.gltf";
  char const *serum_station_enabled_model_relative_filepath = "game\\models\\serum_station_enabled.gltf";
  char const *serum_station_disabled_model_relative_filepath = "game\\models\\serum_station_disabled.gltf";
  char const *ammo_box_model_relative_filepath = "game\\models\\Bullet_Box.gltf";
  char const *serum_absolute_model_relative_filepath = "game\\models\\serum_absolute.gltf";
  char const *ambient_sound_relative_filepath = "game\\sounds\\level0_ambient.mp3";
  char const *shot_sound_relative_filepath = "game\\sounds\\shot-from-an-antique-gun.wav";
  char const *save_theme_sound_relative_filepath = "game\\sounds\\Resident_Evil_1_OST_Save_Room.mp3";
  char const *walking_sound_relative_filepath = "game\\sounds\\walking.wav";
  char const *recycle_sound_relative_filepath = "game\\sounds\\recycle.wav";
  char const *hit_critical_sound_relative_filepath = "game\\sounds\\hit_critical.wav";
  char const *hit_basic_sound_relative_filepath = "game\\sounds\\hit_basic.wav";
  char const *take_serum_sound_relative_filepath = "game\\sounds\\take_serum.wav";
  char const *enemy_idle_sound_relative_filepath = "game\\sounds\\enemy_idle.mp3";
  char const *enemy_notice_sound_relative_filepath = "game\\sounds\\enemy_scream.wav";
  char const *enemy_attack_sound_relative_filepath = "game\\sounds\\enemy_attack.wav";
  char const *recycle_interaction_sound_relative_filepath = "game\\sounds\\recycle_interaction.wav";
  char const *syringe_sound_relative_filepath = "game\\sounds\\syringe.wav";
  char const *reload_interaction_sound_relative_filepath = "game\\sounds\\reload.wav";
  char const *heartbeat_sound_relative_filepath = "game\\sounds\\heartbeat.wav";
  char const *ghost_sound_relative_filepath = "game\\sounds\\ghost_sound.mp3";
  char const *death_sound_relative_filepath = "game\\sounds\\death_music.mp3";
  char const *shot_without_ammo_sound_relative_filepath = "game\\sounds\\shot_without_ammo.wav";
  char const *starting_room_background_sound_relative_filepath = "game\\sounds\\starting_room\\background.mp3";
  char const *boss_fight_sound_relative_filepath = "game\\sounds\\boss_fight.mp3";
  char const *game_font_relative_filepath = "game\\fonts\\IgnotumRegular.ttf";
  char const *hit_0_sound_relative_filepath = "game\\sounds\\hit_0.wav";
  char const *hit_1_sound_relative_filepath = "game\\sounds\\hit_1.wav";
  char const *hit_2_sound_relative_filepath = "game\\sounds\\hit_2.wav";
  char const *level_intro_sound_relative_filepath = "game\\sounds\\intro\\background.wav";
  char const *level_intro_node_relative_filepath = "game\\nodes\\level_intro.crude_node";
  char const *level_menu_node_relative_filepath = "game\\nodes\\level_menu.crude_node";
  char const *level_starting_room_node_relative_filepath = "game\\nodes\\level_starting_room.crude_node";
  char const *level_0_node_relative_filepath = "game\\nodes\\level0.crude_node";
  char const *level_boss_fight_0_node_relative_filepath = "game\\nodes\\level_boss_fight.crude_node";
  char const *level_boss_fight_1_node_relative_filepath = "game\\nodes\\level_boss_fight1.crude_node";
  char const *level_cutscene0_node_relative_filepath = "game\\nodes\\level_cutscene0.crude_node";
  char const *level_cutscene1_node_relative_filepath = "game\\nodes\\level_cutscene1.crude_node";
  char const *level_cutscene2_node_relative_filepath = "game\\nodes\\level_cutscene2.crude_node";
  char const *level_cutscene3_node_relative_filepath = "game\\nodes\\level_cutscene3.crude_node";
  char const *level_cutscene4_node_relative_filepath = "game\\nodes\\level_cutscene4.crude_node";
  char const *starting_room_voiceline0_sound_relative_filepath = "game\\sounds\\starting_room\\voiceline0.wav";
  char const *starting_room_voiceline1_sound_relative_filepath = "game\\sounds\\starting_room\\voiceline1.wav";
  char const *starting_room_voiceline2_sound_relative_filepath = "game\\sounds\\starting_room\\voiceline2.wav";
  char const *starting_room_voiceline3_sound_relative_filepath = "game\\sounds\\starting_room\\voiceline3.wav";
  char const *level_cutscene0_sound_relative_filepath = "game\\sounds\\cutscenes\\cutscene0.wav";
  char const *level_cutscene1_sound_relative_filepath = "game\\sounds\\cutscenes\\cutscene1.mp3";
  char const *level_cutscene2_sound_relative_filepath = "game\\sounds\\cutscenes\\cutscene2.mp3";
  char const *level_cutscene3_sound_relative_filepath = "game\\sounds\\cutscenes\\cutscene3.mp3";
  char const *level_cutscene4_sound_relative_filepath = "game\\sounds\\cutscenes\\cutscene4.mp3";
  char const *level_menu_sound_relative_filepath = "game\\sounds\\cutscenes\\menu.mp3";
  char const *show_2_sound_relative_filepath = "game\\sounds\\shot2.mp3";
  char const *super_criticlal_shot_sound_relative_filepath = "game\\sounds\\super_critical.mp3";

  char const *starting_room_modern_syringe_health_model_relative_filepath = "game\\models\\starting_room\\Plastic_Syringe_Health.gltf";
  char const *starting_room_modern_syringe_drug_model_relative_filepath = "game\\models\\starting_room\\Plastic_Syringe_Drug.gltf";

  uint64 constant_string_buffer_size = 0;
  uint64 working_directory_length = crude_string_length( working_absolute_directory ) + 1;
  constant_string_buffer_size += working_directory_length;
  constant_string_buffer_size += working_directory_length;
  constant_string_buffer_size += working_directory_length + crude_string_length( shaders_relative_directory );
  constant_string_buffer_size += working_directory_length + crude_string_length( render_graph_relative_directory );
  constant_string_buffer_size += working_directory_length + crude_string_length( scene_relative_filepath );
  constant_string_buffer_size += working_directory_length + crude_string_length( techniques_relative_directory );
  constant_string_buffer_size += working_directory_length + crude_string_length( compiled_shaders_relative_directory );
  
  uint64 resources_absolute_directory_length = working_directory_length + crude_string_length( resources_relative_directory );
  constant_string_buffer_size += resources_absolute_directory_length;
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( serum_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( show_2_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( super_criticlal_shot_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( boss_bullet_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( serum_station_enabled_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( serum_station_disabled_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( syringe_drug_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( syringe_health_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( ammo_box_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( ambient_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( shot_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( save_theme_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( walking_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( recycle_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( hit_critical_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( hit_basic_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( take_serum_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( enemy_idle_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( enemy_notice_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( enemy_attack_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( recycle_interaction_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( syringe_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( reload_interaction_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( heartbeat_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( game_font_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( death_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( shot_without_ammo_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( serum_absolute_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( starting_room_background_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( boss_fight_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( hit_0_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( hit_1_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( hit_2_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_intro_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_intro_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( starting_room_voiceline0_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( starting_room_voiceline1_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( starting_room_voiceline2_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( starting_room_voiceline3_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( starting_room_modern_syringe_health_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( starting_room_modern_syringe_drug_model_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene0_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_0_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_boss_fight_0_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_boss_fight_1_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene0_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene1_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene2_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene3_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene4_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene1_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene2_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene3_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_cutscene4_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_menu_sound_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( level_menu_node_relative_filepath );
  constant_string_buffer_size += resources_absolute_directory_length + crude_string_length( ghost_sound_relative_filepath );
  
  //crude_string_buffer_initialize( &game->constant_strings_buffer, constant_string_buffer_size, crude_heap_allocator_pack( &game->allocator ) );
  
  game->working_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s", working_absolute_directory );
  game->shaders_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, shaders_relative_directory );
  game->scene_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, scene_relative_filepath );
  game->render_graph_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, render_graph_relative_directory );
  game->techniques_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, techniques_relative_directory );
  game->compiled_shaders_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, compiled_shaders_relative_directory );
  
  game->resources_absolute_directory = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->working_absolute_directory, resources_relative_directory );
  game->serum_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, serum_model_relative_filepath );
  game->boss_bullet_model_absolute_filepath  = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, boss_bullet_model_relative_filepath );
  game->syringe_drug_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, syringe_drug_model_relative_filepath );
  game->syringe_health_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, syringe_health_model_relative_filepath );
  game->serum_station_enabled_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, serum_station_enabled_model_relative_filepath );
  game->serum_station_disabled_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, serum_station_disabled_model_relative_filepath );
  game->ammo_box_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, ammo_box_model_relative_filepath );
  
  game->starting_room_modern_syringe_health_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, starting_room_modern_syringe_health_model_relative_filepath );
  game->starting_room_modern_syringe_drug_model_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, starting_room_modern_syringe_drug_model_relative_filepath );

  game->ambient_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, ambient_sound_relative_filepath );
  game->shot_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, shot_sound_relative_filepath );
  game->save_theme_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, save_theme_sound_relative_filepath );
  game->walking_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, walking_sound_relative_filepath );
  game->recycle_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, recycle_sound_relative_filepath );
  game->hit_critical_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, hit_critical_sound_relative_filepath );
  game->hit_basic_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, hit_basic_sound_relative_filepath );
  game->take_serum_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, take_serum_sound_relative_filepath );
  game->syringe_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, syringe_sound_relative_filepath );
  game->reload_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, reload_interaction_sound_relative_filepath );
  game->heartbeat_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, heartbeat_sound_relative_filepath );
  game->ghost_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, ghost_sound_relative_filepath );
  game->hit_0_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, hit_0_sound_relative_filepath );
  game->hit_1_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, hit_1_sound_relative_filepath );
  game->hit_2_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, hit_2_sound_relative_filepath );
  game->level_intro_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_intro_sound_relative_filepath );
  game->level_cutscene0_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene0_sound_relative_filepath );
  game->level_cutscene1_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene1_sound_relative_filepath );
  game->level_cutscene2_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene2_sound_relative_filepath );
  game->level_cutscene3_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene3_sound_relative_filepath );
  game->level_cutscene4_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene4_sound_relative_filepath );
  game->level_menu_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_menu_sound_relative_filepath );
  game->starting_room_voiceline0_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, starting_room_voiceline0_sound_relative_filepath );;
  game->starting_room_voiceline1_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, starting_room_voiceline1_sound_relative_filepath );;
  game->starting_room_voiceline2_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, starting_room_voiceline2_sound_relative_filepath );;
  game->starting_room_voiceline3_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, starting_room_voiceline3_sound_relative_filepath );;
  game->show_2_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, show_2_sound_relative_filepath );
  game->super_criticlal_shot_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, super_criticlal_shot_sound_relative_filepath );
  
  game->level_menu_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_menu_node_relative_filepath );
  game->level_intro_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_intro_node_relative_filepath );
  game->level_starting_room_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_starting_room_node_relative_filepath );
  game->level_0_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_0_node_relative_filepath );
  game->level_boss_fight_0_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_boss_fight_0_node_relative_filepath );
  game->level_boss_fight_1_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_boss_fight_1_node_relative_filepath );
  game->level_cutscene0_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene0_node_relative_filepath );
  game->level_cutscene1_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene1_node_relative_filepath );
  game->level_cutscene2_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene2_node_relative_filepath );
  game->level_cutscene3_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene3_node_relative_filepath );
  game->level_cutscene4_node_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, level_cutscene4_node_relative_filepath );

  game->enemy_idle_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, enemy_idle_sound_relative_filepath );
  game->enemy_notice_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, enemy_notice_sound_relative_filepath );
  game->enemy_attack_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, enemy_attack_sound_relative_filepath );
  game->recycle_interaction_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, recycle_interaction_sound_relative_filepath );
  game->death_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, death_sound_relative_filepath );
  game->shot_without_ammo_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, shot_without_ammo_sound_relative_filepath );
  game->starting_room_background_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, starting_room_background_sound_relative_filepath );
  game->boss_fight_sound_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, boss_fight_sound_relative_filepath );

  game->game_font_absolute_filepath = crude_string_buffer_append_use_f( &game->constant_strings_buffer, "%s%s", game->resources_absolute_directory, game_font_relative_filepath );

//#if CRUDE_DEVELOP
//  crude_string_buffer_initialize( &game->debug_strings_buffer, 4096, crude_heap_allocator_pack( &game->allocator ) );
//  crude_string_buffer_initialize( &game->debug_constant_strings_buffer, 4096, crude_heap_allocator_pack( &game->allocator ) );
//#endif
//  crude_string_buffer_initialize( &game->game_strings_buffer, 4096, crude_heap_allocator_pack( &game->allocator ) );
//
//#if CRUDE_DEVELOP
//  game->syringe_spawnpoint_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &game->debug_constant_strings_buffer, "%s%s", game->resources_absolute_directory, "debug\\models\\syringe_spawnpoint_model.gltf" );
//  game->enemy_spawnpoint_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &game->debug_constant_strings_buffer, "%s%s", game->resources_absolute_directory, "debug\\models\\enemy_spawnpoint_model.gltf" );
//  game->syringe_serum_station_active_debug_model_absolute_filepath = crude_string_buffer_append_use_f( &game->debug_constant_strings_buffer, "%s%s", game->resources_absolute_directory, "debug\\models\\syringe_serum_station_active_model.gltf" );
//#endif
}

void
game_deinitialize_constant_strings_
(
  _In_ game_t                                             *game
)
{
#if CRUDE_DEVELOP
  crude_string_buffer_deinitialize( &game->debug_constant_strings_buffer );
  crude_string_buffer_deinitialize( &game->debug_strings_buffer );
#endif
  crude_string_buffer_deinitialize( &game->constant_strings_buffer );
  crude_string_buffer_deinitialize( &game->game_strings_buffer );
}

void
game_initialize_audio_
(
  _In_ game_t                                             *game
)
{
//  crude_sound_creation                                     sound_creation;
//
//  sound_creation = crude_sound_creation_empty( );
//  sound_creation.looping = true;
//  sound_creation.stream = true;
//  sound_creation.decode = true;
//  sound_creation.absolute_filepath = game->death_sound_absolute_filepath;
//  sound_creation.positioning = CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE;
//  game->death_sound_handle = crude_audio_device_create_sound( &game->audio_device, &sound_creation );
}


void
game_deinitialize_audio_
(
  _In_ game_t                                             *game
)
{
//  crude_audio_device_destroy_sound( &game->audio_device, game->death_sound_handle );
}

void
game_initialize_graphics_
(
  _In_ game_t                                            *game
)
{
}

void
game_instance_intialize
(
)
{
  game_instance_ = CRUDE_CAST( game_t*, malloc( sizeof( game_t ) ) );
}

void
game_instance_deintialize
(
)
{
  free( game_instance_ );
}

game_t*
game_instance
(
)
{
  return game_instance_;
}