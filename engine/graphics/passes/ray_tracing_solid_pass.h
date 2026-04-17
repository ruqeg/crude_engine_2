#pragma once

#include <engine/graphics/render_graph.h>
#include <engine/graphics/scene_renderer_resources.h>

#if CRUDE_GFX_RAY_TRACING_SOLID_DEBUG_ENABLED

typedef struct crude_gfx_scene_renderer crude_gfx_scene_renderer;

typedef struct crude_gfx_ray_tracing_solid_pass
{
  crude_gfx_scene_renderer                                *scene_renderer;
} crude_gfx_ray_tracing_solid_pass;

CRUDE_API void
crude_gfx_ray_tracing_solid_pass_initialize
(
  _In_ crude_gfx_ray_tracing_solid_pass                   *pass,
  _In_ crude_gfx_scene_renderer                           *scene_renderer
);

CRUDE_API void
crude_gfx_ray_tracing_solid_pass_deinitialize
(
  _In_ crude_gfx_ray_tracing_solid_pass                   *pass
);

CRUDE_API void
crude_gfx_ray_tracing_solid_pass_render
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
);

CRUDE_API crude_gfx_render_graph_pass_container
crude_gfx_ray_tracing_solid_pass_pack
(
  _In_ crude_gfx_ray_tracing_solid_pass                   *pass
);

#endif /* CRUDE_GFX_RAY_TRACING_SOLID_DEBUG_ENABLED */