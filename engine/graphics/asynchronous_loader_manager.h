#pragma once

#include <engine/graphics/asynchronous_loader.h>

/************************************************
 *
 * Asynchronous Loader Manager Structs
 * 
 ***********************************************/
typedef struct crude_gfx_asynchronous_loader_manager
{
  crude_gfx_asynchronous_loader                           *async_loaders[ CRUDE_ENGINE_ASYNCHRONOUS_LOADERS_MAX_COUNT ];
  uint64                                                   async_loaders_count;
  bool                                                     async_loaders_valid;
  mtx_t                                                    task_mutex;
} crude_gfx_asynchronous_loader_manager;

/************************************************
 *
 * 
 * Asynchronous Loader Manager Functions Declaration
 * 
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_asynchronous_loader_manager_intiailize
(
  _In_ crude_gfx_asynchronous_loader_manager                *manager,
  _In_ crude_task_sheduler                                  *task_sheduler,
  _In_ uint64                                                active_async_loaders_max_count
);

CRUDE_API void
crude_gfx_asynchronous_loader_manager_add_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
);

CRUDE_API void
crude_gfx_asynchronous_loader_manager_remove_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
);

CRUDE_API void
crude_gfx_asynchronous_loader_manager_deintiailize
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager
);