#pragma once

#include <nfd.h>

#include <engine/engine.h>
#include <engine/scene/node_manager.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/core/ecs.h>
#include <engine/platform/platform_components.h>
#include <engine/scene/collisions_resources_manager.h>
#include <engine/physics/physics_debug_system.h>
#include <engine/physics/physics.h>
#include <engine/external/game_debug_system.h>
#include <game/devmenu.h>

typedef enum crude_game_queue_command_type
{
  CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_SCENE,
  CRUDE_GAME_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES,
  CRUDE_GAME_QUEUE_COMMAND_TYPE_COUNT,
} crude_game_queue_command_type;

typedef struct crude_game_queue_command
{
  crude_game_queue_command_type                            type;
  union
  {
    struct 
    {
      nfdu8char_t                                         *filepath;
    } reload_scene;
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

  crude_string_buffer                                      constant_strings_buffer;
  crude_string_buffer                                      debug_strings_buffer;

  crude_devmenu                                            devmenu;

  /* Common */
  crude_heap_allocator                                     allocator;
  crude_heap_allocator                                     resources_allocator;
  crude_heap_allocator                                     cgltf_temporary_allocator;
  crude_stack_allocator                                    temporary_allocator;
  crude_stack_allocator                                    model_renderer_resources_manager_temporary_allocator;
  crude_entity                                             main_node;

  /* Graphics */
  crude_gfx_device                                         gpu;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  crude_gfx_asynchronous_loader                            async_loader;
  crude_gfx_scene_renderer                                 scene_renderer;
  crude_gfx_model_renderer_resources_manager               model_renderer_resources_manager;
#if CRUDE_DEVELOP
  void                                                    *imgui_context;
#endif
  /* Physics */
  crude_physics_resources_manager                          physics_resources_manager;
  crude_collisions_resources_manager                       collision_resources_manager;
  crude_physics                                            physics;
  /* Scene */
  crude_node_manager                                       node_manager;
  /* Window & Input */
  crude_entity                                             platform_node;
  XMFLOAT2                                                 last_unrelative_mouse_position;
  /* Game */
  crude_entity                                             focused_camera_node;
  /* System Context */
#if CRUDE_DEVELOP
  crude_physics_debug_system_context                       physics_debug_system_context;
  crude_game_debug_system_context                          game_debug_system_context;
#endif
  /* Other */
  uint32                                                   framerate;
  float32                                                  last_graphics_update_time;

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
  _In_ game_t                                             *game,
  _In_ nfdu8char_t                                        *filepath
);

CRUDE_API void
game_push_reload_techniques_command
(
  _In_ game_t                                             *game
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