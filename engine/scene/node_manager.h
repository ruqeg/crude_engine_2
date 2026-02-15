#pragma once

#include <engine/core/ecs.h>
#include <engine/core/octree.h>
#include <engine/core/memory.h>
#include <engine/core/string.h>
#include <engine/physics/physics_resources_manager.h>

typedef struct crude_node_manager crude_node_manager;

typedef bool (*crude_scene_parse_json_to_component_func)
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name,
  _In_ crude_node_manager                                 *manager
);

typedef void (*crude_scene_parse_all_components_to_json_func)
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                              *node_components_json,
  _In_ crude_node_manager                                 *manager
);

typedef struct crude_node_manager_creation
{
  crude_physics_resources_manager                         *physics_resources_manager;
  crude_collisions_resources_manager                      *collisions_resources_manager;
  crude_stack_allocator                                   *temporary_allocator;
  crude_heap_allocator                                    *allocator;
  crude_scene_parse_json_to_component_func                 additional_parse_json_to_component_func;
  crude_scene_parse_all_components_to_json_func            additional_parse_all_components_to_json_func;
  char const                                              *resources_absolute_directory;
} crude_node_manager_creation;

typedef struct crude_node_manager
{
  /* Context */
  crude_physics_resources_manager                         *physics_resources_manager;
  crude_collisions_resources_manager                      *collisions_resources_manager;
  crude_stack_allocator                                   *temporary_allocator;
  crude_heap_allocator                                    *allocator;
  crude_scene_parse_json_to_component_func                 additional_parse_json_to_component_func;
  crude_scene_parse_all_components_to_json_func            additional_parse_all_components_to_json_func;
  char const                                              *resources_absolute_directory;
  struct { uint64 key; crude_entity value; }              *hashed_absolute_filepath_to_node;
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
  _In_ crude_node_manager                                 *manager,
  _In_ crude_ecs                                          *world
);

CRUDE_API crude_entity
crude_node_manager_get_node
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_absolute_filepath,
  _In_ crude_ecs                                          *world
);

CRUDE_API void
crude_node_manager_remove_node
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_absolute_filepath,
  _In_ crude_ecs                                          *world
);

CRUDE_API void
crude_node_manager_save_node_by_file_to_file
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_absolute_filepath,
  _In_ char const                                         *saved_absolute_filepath,
  _In_ crude_ecs                                          *world
);

CRUDE_API void
crude_node_manager_save_node_to_file
(
  _In_ crude_node_manager                                 *manager,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_ char const                                         *saved_absolute_filepath
);