#pragma once

#include <engine/core/ecs.h>
#include <engine/core/hashmapstr.h>
#include <engine/core/resource_pool.h>
#include <engine/physics/physics_resource.h>
#include <engine/core/memory.h>
#include <engine/core/string.h>

#if CRUDE_DEVELOP
#include <engine/graphics/model_renderer_resources_manager.h>
#endif

typedef struct crude_physics crude_physics;

typedef struct crude_physics_mesh_shape_container
{
#if CRUDE_DEVELOP
  crude_gfx_model_renderer_resources_instance              debug_model_renderer_resource_instance;
#endif
  JPH::Ref< JPH::Shape >                                   jph_shape_class;
  char                                                     relative_filepath[ 1024 ];
} crude_physics_mesh_shape_container;

typedef struct crude_physics_shapes_manager_creation
{
  crude_heap_allocator                                    *allocator;
  crude_heap_allocator                                    *cgltf_temporary_allocator;
  crude_physics                                           *physics_manager;
  char const                                              *resources_absolute_directory;
#if CRUDE_DEVELOP
  crude_gfx_model_renderer_resources_manager              *model_renderer_resources_manager;
#endif
} crude_physics_shapes_manager_creation;

typedef struct crude_physics_shapes_manager
{
  /* Context */
  crude_physics                                           *physics_manager;
  crude_heap_allocator                                    *allocator;
  crude_heap_allocator                                    *cgltf_temporary_allocator;
  char const                                              *resources_absolute_directory;
#if CRUDE_DEVELOP
  crude_gfx_model_renderer_resources_manager              *model_renderer_resources_manager;
#endif

  /* Common */
  crude_resource_pool                                      mesh_shape_resource_pool;
  crude_string_buffer                                      gltf_absolute_filepath_string_buffer;
  CRUDE_HASHMAPSTR( crude_physics_mesh_shape_handle )     *mesh_shape_relative_filepath_to_hadle;
} crude_physics_shapes_manager;

CRUDE_API void
crude_physics_shapes_manager_initialize
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ crude_physics_shapes_manager_creation const        *creation
);

CRUDE_API void
crude_physics_shapes_manager_deinitialize
(
  _In_ crude_physics_shapes_manager                       *manager
);

CRUDE_API crude_physics_mesh_shape_handle
crude_physics_shapes_manager_get_mesh_shape_handle
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ char const                                         *relative_filepath
);

CRUDE_API crude_physics_mesh_shape_container*
crude_physics_shapes_manager_access_mesh_shape
(
  _In_ crude_physics_shapes_manager                       *manager,
  _In_ crude_physics_mesh_shape_handle                     handle
);

CRUDE_API void
crude_physics_shapes_manager_clear
(
  _In_ crude_physics_shapes_manager                       *manager
);