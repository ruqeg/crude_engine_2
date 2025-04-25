#pragma once

#include <flecs.h>

#include <scene/entity.h>
#include <resources/gltf_loader.h>
#include <TaskScheduler_c.h>

typedef struct crude_render_create
{
  uint16            max_frames;
  char const       *vk_application_name;
  uint32            vk_application_version;
  crude_allocator   allocator;
} crude_render_create;

typedef struct crude_renderer_component
{
  crude_gfx_device            *gpu;
  crude_gfx_renderer              *renderer;
  crude_gfx_asynchronous_loader *async_loader;
  crude_scene                 *scene;
  crude_entity                 camera;
  enkiTaskScheduler*           ets;
  enkiPinnedTask* pinned_task_run_pinned_task_loop;
  enkiPinnedTask* async_load_task;
} crude_renderer_component;

CRUDE_API ECS_COMPONENT_DECLARE( crude_render_create );
CRUDE_API ECS_COMPONENT_DECLARE( crude_renderer_component );

CRUDE_API void
crude_render_componentsImport
(
  ecs_world_t *world
);