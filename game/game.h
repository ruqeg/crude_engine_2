#pragma once

#include <nfd.h>

#include <engine/engine.h>
#include <engine/scene/node_manager.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/core/ecs.h>
#include <engine/platform/platform_components.h>
#include <engine/scene/collisions_resources_manager.h>
#include <engine/physics/physics_debug_system.h>
#include <engine/physics/physics_system.h>
#include <engine/external/game_debug_system.h>
#include <engine/external/game_components.h>
#include <engine/graphics/imgui.h>
#include <engine/audio/audio_system.h>
#include <game/graphics/passes/game_postprocessing_pass.h>
#include <game/game_menu.h>
#include <game/devmenu.h>

typedef struct crude_game_creation
{
  char const                                              *scene_relative_filepath;
  char const                                              *render_graph_relative_directory;
  char const                                              *resources_relative_directory;
  char const                                              *techniques_relative_directory;
  char const                                              *shaders_relative_directory;
  char const                                              *compiled_shaders_relative_directory;
  char const                                              *working_absolute_directory;
  crude_engine                                            *engine;
  uint32                                                   framerate;
} crude_game_creation;

typedef struct game_t
{
  /* Context */
  crude_engine                                            *engine;
  
  char const                                              *scene_absolute_filepath;
  char const                                              *render_graph_absolute_directory;
  char const                                              *techniques_absolute_directory;
  char const                                              *resources_absolute_directory;
  char const                                              *shaders_absolute_directory;
  char const                                              *compiled_shaders_absolute_directory;
  char const                                              *working_absolute_directory;
  char const                                              *enemy_node_absolute_filepath;
  char const                                              *serum_station_node_absolute_filepath;
  char const                                              *boss_bullet_node_absolute_filepath;

  char const                                              *boss_bullet_model_absolute_filepath;
  char const                                              *serum_model_absolute_filepath;
  char const                                              *starting_room_modern_syringe_health_model_absolute_filepath;
  char const                                              *starting_room_modern_syringe_drug_model_absolute_filepath;
  char const                                              *syringe_drug_model_absolute_filepath;
  char const                                              *syringe_health_model_absolute_filepath;
  char const                                              *serum_station_enabled_model_absolute_filepath;
  char const                                              *serum_station_disabled_model_absolute_filepath;
  char const                                              *ammo_box_model_absolute_filepath;
  char const                                              *ambient_sound_absolute_filepath;
  char const                                              *save_theme_sound_absolute_filepath;
  char const                                              *walking_sound_absolute_filepath;
  char const                                              *shot_sound_absolute_filepath;
  char const                                              *shot_without_ammo_sound_absolute_filepath;
  char const                                              *starting_room_background_sound_absolute_filepath;
  char const                                              *boss_fight_sound_absolute_filepath;
  char const                                              *level_intro_sound_absolute_filepath;
  char const                                              *level_cutscene0_sound_absolute_filepath;
  char const                                              *level_cutscene1_sound_absolute_filepath;
  char const                                              *level_cutscene2_sound_absolute_filepath;
  char const                                              *level_cutscene3_sound_absolute_filepath;
  char const                                              *level_cutscene4_sound_absolute_filepath;
  char const                                              *level_menu_sound_absolute_filepath;
  char const                                              *recycle_sound_absolute_filepath;
  char const                                              *hit_critical_sound_absolute_filepath;
  char const                                              *hit_basic_sound_absolute_filepath;
  char const                                              *take_serum_sound_absolute_filepath;
  char const                                              *enemy_idle_sound_absolute_filepath;
  char const                                              *enemy_notice_sound_absolute_filepath;
  char const                                              *enemy_attack_sound_absolute_filepath;
  char const                                              *recycle_interaction_sound_absolute_filepath;
  char const                                              *syringe_sound_absolute_filepath;
  char const                                              *reload_sound_absolute_filepath;
  char const                                              *death_sound_absolute_filepath;
  char const                                              *heartbeat_sound_absolute_filepath;
  char const                                              *ghost_sound_absolute_filepath;
  char const                                              *hit_0_sound_absolute_filepath;
  char const                                              *hit_1_sound_absolute_filepath;
  char const                                              *hit_2_sound_absolute_filepath;
  char const                                              *starting_room_voiceline0_sound_absolute_filepath;
  char const                                              *starting_room_voiceline1_sound_absolute_filepath;
  char const                                              *starting_room_voiceline2_sound_absolute_filepath;
  char const                                              *starting_room_voiceline3_sound_absolute_filepath;
  char const                                              *show_2_sound_absolute_filepath;
  char const                                              *super_criticlal_shot_sound_absolute_filepath;

  /* */
  char const                                              *level_menu_node_absolute_filepath;
  char const                                              *level_intro_node_absolute_filepath;
  char const                                              *level_starting_room_node_absolute_filepath;
  char const                                              *level_0_node_absolute_filepath;
  char const                                              *level_boss_fight_0_node_absolute_filepath;
  char const                                              *level_boss_fight_1_node_absolute_filepath;
  char const                                              *level_cutscene0_node_absolute_filepath;
  char const                                              *level_cutscene1_node_absolute_filepath;
  char const                                              *level_cutscene2_node_absolute_filepath;
  char const                                              *level_cutscene3_node_absolute_filepath;
  char const                                              *level_cutscene4_node_absolute_filepath;
  
  char const                                              *game_font_absolute_filepath;

  crude_string_buffer                                      constant_strings_buffer;
  crude_string_buffer                                      game_strings_buffer;

  char const                                              *current_scene_absolute_filepath;


  crude_game_menu                                          game_menu;

  /* Common */
  crude_entity                                             main_node;
  crude_entity                                             template_boss_bullet_node;
  crude_entity                                             template_enemy_node;
  crude_entity                                             template_serum_station_node;
  crude_entity                                             player_node;
  float32                                                  sensetivity;

  /* Game */
  crude_entity                                             focused_camera_node;

  crude_sound_handle                                       death_sound_handle;
  bool                                                     death_screen;
  char const                                              *death_reason;
  XMFLOAT4                                                 death_overlap_color;
  /* Other */
  float32                                                  time;

#if CRUDE_DEVELOP
  crude_devmenu                                            devmenu;

  crude_string_buffer                                      debug_strings_buffer;
  crude_string_buffer                                      debug_constant_strings_buffer;

  char const                                              *syringe_serum_station_active_debug_model_absolute_filepath;
  char const                                              *syringe_spawnpoint_debug_model_absolute_filepath;
  char const                                              *enemy_spawnpoint_debug_model_absolute_filepath;

  crude_game_debug_system_context                          game_debug_system_context;
#endif
} game_t;

CRUDE_API void
game_initialize
(
  _In_ game_t                                             *game,
  _In_ crude_game_creation const                          *creation
);

CRUDE_API void
game_postupdate
(
  _In_ game_t                                             *game
);

CRUDE_API void
game_deinitialize
(
  _In_ game_t                                             *game
);

CRUDE_API void
game_player_set_item
(
  _In_ game_t                                             *game,
  _In_ crude_player                                       *player,
  _In_ uint32                                              slot,
  _In_ crude_game_item                                     item
);

CRUDE_API void
game_instance_intialize
(
);

CRUDE_API void
game_instance_deintialize
(
);

CRUDE_API game_t*
game_instance
(
);