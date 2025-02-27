#pragma once

#include <flecs.h>

#include <graphics/gpu_device.h>

typedef struct crude_render_create
{
  uint16            max_frames;
  char const       *vk_application_name;
  uint32            vk_application_version;
  crude_allocator   allocator;
} crude_render_create;

typedef struct crude_renderer
{
  crude_gpu_device            *gpu;
  crude_pipeline_handle        pipeline;
} crude_renderer;

CRUDE_API ECS_COMPONENT_DECLARE( crude_render_create );
CRUDE_API ECS_COMPONENT_DECLARE( crude_renderer );

CRUDE_API void
crude_render_componentsImport
(
  ecs_world_t *world
);