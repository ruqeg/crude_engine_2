#pragma once

#include <TaskScheduler_c.h>

#include <core/memory.h>
#include <core/ecs.h>

typedef struct crude_gfx_graphics_creation
{
  enkiTaskScheduler                                       *task_sheduler;
  crude_allocator_container                                allocator_container;
  crude_stack_allocator                                   *temporary_allocator;
  crude_entity                                             camera;
} crude_gfx_graphics_creation;

typedef struct crude_gfx_graphics_handle
{
  void                                                    *value;
} crude_gfx_graphics_handle;

CRUDE_API CRUDE_ECS_COMPONENT_DECLARE( crude_gfx_graphics_handle );
CRUDE_API CRUDE_ECS_COMPONENT_DECLARE( crude_gfx_graphics_creation );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_graphics_components );