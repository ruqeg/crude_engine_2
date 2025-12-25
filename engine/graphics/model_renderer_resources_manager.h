#pragma once

#include <engine/graphics/gpu_device.h>

#include <engine/scene/scene_components.h>
#include <engine/graphics/model_renderer_resources.h>
#include <engine/graphics/asynchronous_loader.h>
#include <engine/graphics/gpu_memory.h>

typedef struct crude_gfx_model_renderer_resources_manager_creation
{
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
  crude_gfx_memory_allocation                             *buffers;
  
  /***********************
   * Common Mesh & Meshlets CPU & GPU Data
   **********************/

  struct { uint64 key; crude_gfx_model_renderer_resources value; } *model_hashed_name_to_model_renderer_resource;

  /* For meshlet pipelines */
  crude_gfx_memory_allocation                              meshlets_hga;
  crude_gfx_memory_allocation                              meshlets_vertices_hga;
  crude_gfx_memory_allocation                              meshlets_vertices_indices_hga;
  crude_gfx_memory_allocation                              meshlets_triangles_indices_hga;
  uint64                                                   total_meshlets_count;
  uint64                                                   total_meshlets_vertices_count;
  uint64                                                   total_meshlets_vertices_indices_count;
  uint64                                                   total_meshlets_triangles_indices_count;

  /* Common */
  crude_gfx_memory_allocation                              meshes_draws_hga;
  crude_gfx_memory_allocation                              meshes_bounds_hga;
  uint64                                                   total_meshes_count;
} crude_gfx_model_renderer_resources_manager;

CRUDE_API void
crude_gfx_model_renderer_resources_manager_intialize
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager,
  _In_ crude_gfx_model_renderer_resources_manager_creation const *creation
);

CRUDE_API void
crude_gfx_model_renderer_resources_manager_deintialize
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager
);

CRUDE_API void
crude_gfx_model_renderer_resources_manager_clear
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager
);

CRUDE_API crude_gfx_model_renderer_resources
crude_gfx_model_renderer_resources_manager_get_gltf_model
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager,
  _In_ char const                                          *filepath,
  _Out_opt_ bool                                           *model_initialized
);

CRUDE_API void
crude_gfx_model_renderer_resources_manager_wait_till_uploaded
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager
);