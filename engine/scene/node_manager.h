#pragma once

#include <engine/core/ecs.h>
#include <engine/core/memory.h>
#include <engine/core/string.h>
#include <engine/core/hashmapstr.h>
#include <engine/graphics/model_renderer_resources_manager.h>
#include <engine/physics/physics.h>

typedef struct crude_node_manager crude_node_manager;

typedef void (*crude_node_manager_select_camera)
( 
  _In_ void                                               *ctx,
  _In_ crude_entity                                        camera_node
);

typedef struct crude_node_manager_node_json
{
  cJSON                                                   *json;
  char                                                     relative_filepath[ CRUDE_NODE_RELATIVE_FILEPATH_LENGTH_MAX ];
} crude_node_manager_node_json;

typedef struct crude_node_manager_creation
{
  crude_physics                                           *physics_manager;
  crude_stack_allocator                                   *temporary_allocator;
  crude_heap_allocator                                    *allocator;
  crude_components_serialization_manager                  *components_serialization_manager;
  crude_node_manager_select_camera                         select_camera_func;
  void                                                    *select_camera_ctx;
  crude_gfx_model_renderer_resources_manager              *model_renderer_resources_manager;
  char const                                              *resources_absolute_directory;
} crude_node_manager_creation;

typedef struct crude_node_manager
{
  /* Context */
  crude_physics                                           *physics_manager;
  crude_gfx_model_renderer_resources_manager              *model_renderer_resources_manager;
  crude_components_serialization_manager                  *components_serialization_manager;
  crude_stack_allocator                                   *temporary_allocator;
  crude_heap_allocator                                    *allocator;

  /* Data */
  crude_node_manager_select_camera                         select_camera_func;
  void                                                    *select_camera_ctx;
  char const                                              *resources_absolute_directory;
  CRUDE_HASHMAPSTR( crude_node_manager_node_json )        *relative_filepath_to_node_json;
  crude_string_buffer                                      absolute_filepath_string_buffer;
} crude_node_manager;

CRUDE_API void
crude_node_manager_initialize
(
  _In_ crude_node_manager                                 *manager,
  _In_ crude_node_manager_creation const                  *creation
);

CRUDE_API void
crude_node_manager_deinitialize
(
  _In_ crude_node_manager                                 *manager
);

CRUDE_API void
crude_node_manager_clear
(
  _In_ crude_node_manager                                 *manager
);

CRUDE_API crude_entity
crude_node_manager_create_node
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_realtive_filepath,
  _In_ crude_ecs                                          *world
);

CRUDE_API void
crude_node_manager_save_node_to_file
(
  _In_ crude_node_manager                                 *manager,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_ char const                                         *saved_relative_filepath
);