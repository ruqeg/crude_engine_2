#pragma once

#include <nfd.h>

#include <engine/engine.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/core/ecs.h>
#include <engine/platform/platform_components.h>
#include <engine/scene/collisions_resources_manager.h>
#include <editor/devgui.h>
#include <engine/physics/physics_resources_manager.h>
#include <engine/physics/physics_debug_system.h>
#include <engine/external/game_debug_system.h>

typedef enum crude_editor_queue_command_type
{
  CRUDE_EDITOR_QUEUE_COMMAND_TYPE_RELOAD_SCENE,
  CRUDE_EDITOR_QUEUE_COMMAND_TYPE_RELOAD_TECHNIQUES,
  CRUDE_EDITOR_QUEUE_COMMAND_TYPE_COUNT,
} crude_editor_queue_command_type;

typedef struct crude_editor_queue_command
{
  crude_editor_queue_command_type                          type;
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
} crude_editor_queue_command;

typedef struct crude_editor_creation
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
} crude_editor_creation;

typedef struct crude_editor
{
  crude_engine                                            *engine;
  
  char const                                              *scene_absolute_filepath;
  char const                                              *render_graph_absolute_directory;
  char const                                              *techniques_absolute_directory;
  char const                                              *resources_absolute_directory;
  char const                                              *shaders_absolute_directory;
  char const                                              *compiled_shaders_absolute_directory;
  char const                                              *working_absolute_directory;

  crude_string_buffer                                      constant_strings_buffer;
  crude_string_buffer                                      dynamic_strings_buffer;
  crude_string_buffer                                      debug_strings_buffer;

  /* Common */
  crude_heap_allocator                                     allocator;
  crude_heap_allocator                                     resources_allocator;
  crude_heap_allocator                                     cgltf_temporary_allocator;
  crude_stack_allocator                                    temporary_allocator;
  crude_stack_allocator                                    model_renderer_resources_manager_temporary_allocator;
  crude_entity                                             main_node;

  crude_physics_resources_manager                          physics_resouces_manager;
  crude_collisions_resources_manager                       collision_resouces_manager;

  /* Graphics */
  crude_gfx_device                                         gpu;
  crude_gfx_render_graph                                   render_graph;
  crude_gfx_render_graph_builder                           render_graph_builder;
  crude_gfx_asynchronous_loader                            async_loader;
  crude_gfx_scene_renderer                                 scene_renderer;
  crude_gfx_model_renderer_resources_manager               model_renderer_resources_manager;
  void                                                    *imgui_context;
  /* Dev */
  crude_devgui                                             devgui;
  /* Scene */
  crude_node_manager                                       node_manager;
  /* Window & Input */
  crude_entity                                             platform_node;
  XMFLOAT2                                                 last_unrelative_mouse_position;
  /* Game */
  crude_entity                                             focused_camera_node;
  crude_entity                                             editor_camera_node;
  crude_entity                                             selected_node;
  crude_devgui_added_node_data                             added_node_data;
  crude_entity                                             node_to_add;
  crude_entity                                             node_to_remove;
  crude_entity                                             node_to_dublicate;
  /* System Context */
#if CRUDE_DEVELOP
  crude_physics_debug_system_context                       physics_debug_system_context;
  crude_game_debug_system_context                          game_debug_system_context;
#endif
  /* Other */
  uint32                                                   framerate;
  float32                                                  last_graphics_update_time;

  crude_editor_queue_command                              *commands_queue;
} crude_editor;

CRUDE_API void
crude_editor_initialize
(
  _In_ crude_editor                                       *editor,
  _In_ crude_editor_creation const                        *creation
);

CRUDE_API void
crude_editor_postupdate
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_deinitialize
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_push_reload_scene_command
(
  _In_ crude_editor                                       *editor,
  _In_ nfdu8char_t                                        *filepath
);

CRUDE_API void
crude_editor_push_reload_techniques_command
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_instance_intialize
(
);

CRUDE_API void
crude_editor_instance_deintialize
(
);

CRUDE_API crude_editor*
crude_editor_instance
(
);