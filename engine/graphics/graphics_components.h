#pragma once

#include <flecs.h>

#include <scene/entity.h>
#include <scene/scene.h>
#include <TaskScheduler_c.h>

typedef struct crude_graphics_creation
{
  crude_allocator_container                                allocator_container;
} crude_graphics_creation;

typedef struct crude_graphics_component
{
  crude_gfx_device                                         *gpu;
  crude_gfx_renderer                                       *renderer;
  crude_gfx_asynchronous_loader                            *async_loader;
  crude_gltf_scene                                         *scene;
  crude_gfx_render_graph                                   *render_graph;
  crude_gfx_render_graph_builder                           *render_graph_builder;
  crude_entity                                              camera;
  enkiTaskScheduler                                        *ets;
  enkiPinnedTask                                           *pinned_task_run_pinned_task_loop;
  enkiPinnedTask                                           *async_load_task;
} crude_graphics_component;

CRUDE_API ECS_COMPONENT_DECLARE( crude_graphics_creation );
CRUDE_API ECS_COMPONENT_DECLARE( crude_graphics_component );

CRUDE_API void
crude_graphics_componentsImport
(
  ecs_world_t *world
);