#pragma once

#include <graphics/gpu_device.h>

#include <scene/scene_components.h>
#include <graphics/model_renderer_resources.h>
#include <graphics/asynchronous_loader.h>

typedef struct crude_gfx_model_renderer_resources_manager_creation
{
  void                                                    *world;
  crude_gfx_asynchronous_loader                           *async_loader;
  crude_heap_allocator                                    *allocator;
  crude_heap_allocator                                    *cgltf_temporary_allocator;
  crude_stack_allocator                                   *temporary_allocator;
} crude_gfx_model_renderer_resources_manager_creation;

typedef struct crude_gfx_model_renderer_resources_manager
{
  /***********************
   * Context 
   **********************/
  void                                                    *world;
  crude_gfx_device                                        *gpu;
  crude_gfx_asynchronous_loader                           *async_loader;
  crude_heap_allocator                                    *allocator;
  crude_heap_allocator                                    *cgltf_temporary_allocator;
  crude_stack_allocator                                   *temporary_allocator;
  
  /***********************
   * Common CPU & GPU Data
   **********************/
  crude_gfx_sampler_handle                                *samplers;
  crude_gfx_texture_handle                                *images;
  crude_gfx_buffer_handle                                 *buffers;
  
  /***********************
   * Common Mesh & Meshlets CPU & GPU Data
   **********************/
  uint64                                                   total_meshes_count;
  uint64                                                   total_meshlets_count;
  uint64                                                   total_meshlets_vertices_count;
  uint64                                                   total_meshlets_vertices_indices_count;
  uint64                                                   total_meshlets_triangles_indices_count;

  struct { uint64 key; crude_gfx_model_renderer_resources value; } *model_hashed_name_to_model_renderer_resource;

  crude_gfx_buffer_handle                                  meshlets_sb;
  crude_gfx_buffer_handle                                  meshlets_vertices_sb;
  crude_gfx_buffer_handle                                  meshlets_vertices_indices_sb;
  crude_gfx_buffer_handle                                  meshlets_triangles_indices_sb;
  crude_gfx_buffer_handle                                  meshes_draws_sb;
  crude_gfx_buffer_handle                                  meshes_bounds_sb;
} crude_gfx_model_renderer_resources_manager;

CRUDE_API void
crude_gfx_model_renderer_resources_manager_intialize
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
	_In_ crude_gfx_model_renderer_resources_manager_creation const *creation
);

CRUDE_API void
crude_gfx_model_renderer_resources_manager_deintialize
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager
);

CRUDE_API crude_gfx_model_renderer_resources
crude_gfx_model_renderer_resources_manager_add_gltf_model
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
	_In_ char const																					*filepath
);

CRUDE_API void
crude_gfx_model_renderer_resources_manager_wait_till_uploaded
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager
);