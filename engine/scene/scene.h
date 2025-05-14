#pragma once

#include <core/ecs.h>
#include <core/memory.h>

typedef struct crude_scene_creation
{
  char const                                              *resources_path;
  char const                                              *json_name;
  crude_allocator_container                                allocator_container;
  crude_stack_allocator                                   *temporary_allocator;
} crude_scene_creation;

typedef struct crude_scene
{
  crude_entity                                            *nodes;
} crude_scene;