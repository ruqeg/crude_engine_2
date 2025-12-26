#pragma once

#include <nfd.h>

#include <engine/engine.h>
#include <engine/graphics/scene_renderer.h>
#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/collisions_resources_manager.h>
#include <editor/devgui.h>
#include <engine/physics/physics_resources_manager.h>
#include <engine/physics/physics_debug_ecs.h>
#include <engine/scene/scene_ecs.h>
#include <engine/scene/scripts/free_camera_ecs.h>
#include <engine/external/game_ecs.h>
#include <engine/external/game_debug_ecs.h>

typedef struct crude_editor
{
  crude_engine                                            *engine;

  crude_string_buffer                                      debug_strings_buffer;
  crude_string_buffer                                      debug_constant_strings_buffer;

  /* Common */
  crude_entity                                             main_node;
  
  char const                                              *syringe_spawnpoint_debug_model_absolute_filepath;
  char const                                              *enemy_spawnpoint_debug_model_absolute_filepath;

  /* Dev */
  crude_devgui                                             devgui;
  /* Game */
  crude_entity                                             editor_camera_node;
  crude_entity                                             selected_node;
  crude_devgui_added_node_data                             added_node_data;
  crude_entity                                             node_to_add;
  crude_entity                                             node_to_remove;
  crude_entity                                             node_to_dublicate;
  /* System Context */
  crude_free_camera_system_context                         free_camera_system_context;
  crude_physics_debug_system_context                       physics_debug_system_context;
  crude_game_debug_system_context                          game_debug_system_context;
} crude_editor;

CRUDE_API void
crude_editor_initialize
(
  _In_ crude_editor                                       *editor,
  _In_ crude_engine                                       *engine,
  _In_ char const                                         *working_directory
);

CRUDE_API void
crude_editor_deinitialize
(
  _In_ crude_editor                                       *editor
);

CRUDE_API void
crude_editor_update
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