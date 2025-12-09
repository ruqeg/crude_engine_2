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
#include <engine/audio/audio_system.h>
#include <game/graphics/passes/game_postprocessing_pass.h>
#include <game/devmenu.h>

typedef enum crude_game_queue_command_type
{
  CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_SCENE,
  CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES,
  CRUDE_GAME_QUEUE_COMMAND_TYPE_ENABLE_RANDOM_SERUM_STATION, /* Wtf is it doing here ahaha */
  CRUDE_GAME_QUEUE_COMMAND_TYPE_COUNT,
} crude_game_queue_command_type;

typedef struct crude_game_queue_command
{
  crude_game_queue_command_type                            type;
  union
  {
    struct 
    {
    } reload_scene;
    struct 
    {
      crude_entity                                         ignored_serum_station;
    } enable_random_serum_station;
    struct 
    {
    } reload_techniques;
  };
} crude_game_queue_command;

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

  char const                                              *serum_model_absolute_filepath;
  char const                                              *syringe_drug_model_absolute_filepath;
  char const                                              *syringe_health_model_absolute_filepath;
  char const                                              *serum_station_enabled_model_absolute_filepath;
  char const                                              *serum_station_disabled_model_absolute_filepath;
  char const                                              *ammo_box_model_absolute_filepath;
  char const                                              *ambient_sound_absolute_filepath;
  char const                                              *save_theme_sound_absolute_filepath;
  char const                                              *walking_sound_absolute_filepath;
  char const                                              *shot_sound_absolute_filepath;
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
  char const                                              *heartbeat_sound_absolute_filepath;
#if CRUDE_DEVELOP
  char const                                              *syringe_serum_station_active_debug_model_absolute_filepath;
  char const                                              *syringe_spawnpoint_debug_model_absolute_filepath;
  char const                                              *enemy_spawnpoint_debug_model_absolute_filepath;
#endif

  crude_string_buffer                                      constant_strings_buffer;
  crude_string_buffer                                      game_strings_buffer;
#if CRUDE_DEVELOP
  crude_string_buffer                                      debug_strings_buffer;
  crude_string_buffer                                      debug_constant_strings_buffer;
#endif
  char                                                     current_scene_absolute_filepath[ 1024 ];

  crude_devmenu                                            devmenu;

  /* Common */
  crude_heap_allocator                                     allocator;
  crude_heap_allocator                                     resources_allocator;
  crude_heap_allocator                                     cgltf_temporary_allocator;
  crude_stack_allocator                                    temporary_allocator;
  crude_stack_allocator                                    model_renderer_resources_manager_temporary_allocator;
  crude_entity                                             main_node;
  crude_entity                                             template_enemy_node;
  crude_entity                                             template_serum_station_node;
  crude_entity                                             player_node;

  /* Graphics */
  crude_gfx_device                                         gpu;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  crude_gfx_asynchronous_loader                            async_loader;
  crude_gfx_scene_renderer                                 scene_renderer;
  crude_gfx_model_renderer_resources_manager               model_renderer_resources_manager;
  crude_gfx_game_postprocessing_pass                       game_postprocessing_pass;
#if CRUDE_DEVELOP
  void                                                    *imgui_context;
#endif
  /* Physics */
  crude_physics_resources_manager                          physics_resources_manager;
  crude_collisions_resources_manager                       collision_resources_manager;
  crude_physics                                            physics;
  /* Audio */
  crude_audio_device                                       audio_device;
  crude_audio_system_context                               audio_system_context;
  /* Scene */
  crude_node_manager                                       node_manager;
  /* Window & Input */
  crude_entity                                             platform_node;
  XMFLOAT2                                                 last_unrelative_mouse_position;
  /* Game */
  crude_entity                                             focused_camera_node;
  crude_sound_handle                                       ambient_sound_handle;
  crude_sound_handle                                       save_theme_sound_handle;
  crude_sound_handle                                       shot_sound_handle;
  crude_sound_handle                                       walking_sound_handle;
  crude_sound_handle                                       recycle_sound_handle;
  crude_sound_handle                                       hit_critical_sound_handle;
  crude_sound_handle                                       hit_basic_sound_handle;
  crude_sound_handle                                       take_serum_sound_handle;
  crude_sound_handle                                       recycle_interaction_sound_handle;
  crude_sound_handle                                       syringe_sound_handle;
  crude_sound_handle                                       reload_sound_handle;
  crude_sound_handle                                       heartbeat_sound_handle;
  /* System Context */
  crude_physics_system_context                             physics_system_context;
#if CRUDE_DEVELOP
  crude_physics_debug_system_context                       physics_debug_system_context;
  crude_game_debug_system_context                          game_debug_system_context;
#endif
  /* Other */
  uint32                                                   framerate;
  float32                                                  last_graphics_update_time;
  float32                                                  time;
  float32                                                  graphics_time;

  crude_game_queue_command                                *commands_queue;
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
game_push_reload_scene_command
(
  _In_ game_t                                             *game
);

CRUDE_API void
game_push_reload_techniques_command
(
  _In_ game_t                                             *game
);

CRUDE_API void
game_push_enable_random_serum_station_command
(
  _In_ game_t                                             *game,
  _In_ crude_entity                                        ignored_serum_station
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