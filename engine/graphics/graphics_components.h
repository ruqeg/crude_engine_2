#pragma once

#include <flecs.h>

#include <scene/entity.h>
#include <scene/scene.h>
#include <TaskScheduler_c.h>

typedef struct crude_gfx_graphics_creation
{
  enkiTaskScheduler                                       *task_sheduler;
  crude_allocator_container                                allocator_container;
  crude_stack_allocator                                   *temporary_allocator;
} crude_gfx_graphics_creation;

typedef struct crude_gfx_graphics_handle
{
  void                                                    *value;
} crude_gfx_graphics_handle;

CRUDE_API ECS_COMPONENT_DECLARE( crude_gfx_graphics_handle );
CRUDE_API ECS_COMPONENT_DECLARE( crude_gfx_graphics_creation );

CRUDE_API void
crude_graphics_componentsImport
(
  ecs_world_t *world
);