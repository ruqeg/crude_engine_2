#pragma once

#include <graphics/gpu_device.h>
#include <graphics/renderer.h>
#include <graphics/render_graph.h>
#include <graphics/asynchronous_loader.h>

typedef struct crude_gfx_asynchronous_loader_manager_creation
{
  void                                                    *task_sheduler;
  crude_allocator_container                                allocator_container;
} crude_gfx_asynchronous_loader_manager_creation;

typedef struct crude_gfx_asynchronous_loader_manager
{
  crude_gfx_asynchronous_loader                          **async_loaders;
  bool                                                     async_loaders_valid;
  void                                                    *async_loader_task;
  void                                                    *task_sheduler;
  mtx_t                                                    task_mutex;
} crude_gfx_asynchronous_loader_manager;

CRUDE_API void
crude_gfx_async_loader_task_manager_intiailize
(
  _In_ crude_gfx_asynchronous_loader_manager                *manager,
  _In_ crude_gfx_asynchronous_loader_manager_creation const *creation
);

CRUDE_API void
crude_gfx_async_loader_task_manager_add_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
);

CRUDE_API void
crude_gfx_async_loader_task_manager_remove_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
);

CRUDE_API void
crude_gfx_async_loader_task_manager_deintiailize
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager
);