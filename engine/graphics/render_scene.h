#pragma once

#include <graphics/asynchronous_loader.h>

typedef struct crude_gfx_scene_graph
{
  int a;
} crude_gfx_scene_graph;

typedef void (*crude_gfx_render_scene_initialize)( void *ctx, char const *filename, char const *path, crude_heap_allocator *resident_allocator, crude_stack_allocator *temp_allocator, crude_gfx_asynchronous_loader *async_loader );
typedef void (*crude_gfx_render_scene_deinitialize)( void *ctx, crude_gfx_renderer *renderer );
typedef void (*crude_gfx_render_scene_register_render_passes)( void *ctx, char const *filename, char const *path, crude_heap_allocator *resident_allocator, crude_stack_allocator *temp_allocator, crude_gfx_asynchronous_loader *async_loader );
typedef void (*crude_gfx_render_scene_prepare_draws)( void *ctx, char const *filename, char const *path, crude_heap_allocator *resident_allocator, crude_stack_allocator *temp_allocator, crude_gfx_asynchronous_loader *async_loader );
typedef void (*crude_gfx_render_scene_upload_materials)( void *ctx, char const *filename, char const *path, crude_heap_allocator *resident_allocator, crude_stack_allocator *temp_allocator, crude_gfx_asynchronous_loader *async_loader );
//typedef void (*crude_gfx_render_scene_submit_draw_task)( void *ctx, ImGuiService* imgui, GPUProfiler* gpu_profiler, enki::TaskScheduler* task_scheduler );

typedef struct crude_gfx_render_scene
{
  crude_gfx_render_scene_initialize                        initialize;
  crude_gfx_render_scene_deinitialize                      deinitialize;
  crude_gfx_render_scene_register_render_passes            render_pass;
  crude_gfx_render_scene_prepare_draws                     prepare_draws;
  crude_gfx_render_scene_upload_materials                  upload_materials;
  crude_gfx_scene_graph*                                   scene_graph;
  crude_gfx_buffer_handle                                  scene_cb;
  float32                                                  global_scale;
} crude_gfx_render_scene;