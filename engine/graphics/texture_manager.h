#pragma once

#include <engine/graphics/asynchronous_loader.h>

typedef struct crude_gfx_texture_manager
{
  crude_gfx_asynchronous_loader                           *asynchronous_loader;
  crude_heap_allocator                                    *texture_manager_allocator;
  CRUDE_HASHMAPSTR( crude_gfx_texture_handle )            *texture_relative_filepath_to_handle;
} crude_gfx_texture_manager;

CRUDE_API void
crude_gfx_texture_manager_initialize
(
  _In_ crude_gfx_texture_manager                          *manager,
  _In_ crude_gfx_asynchronous_loader                      *asynchronous_loader,
  _In_ crude_heap_allocator                               *texture_manager_allocator
);

CRUDE_API void
crude_gfx_texture_manager_deinitialize
(
  _In_ crude_gfx_texture_manager                          *manager
);

CRUDE_API void
crude_gfx_texture_manager_clear
(
  _In_ crude_gfx_texture_manager                          *manager
);

CRUDE_API crude_gfx_texture_handle
crude_gfx_texture_manager_get_texture
(
  _In_ crude_gfx_texture_manager                          *manager,
  _In_ char const                                         *relative_filepath,
  _In_ char const                                         *absolute_filepath
);