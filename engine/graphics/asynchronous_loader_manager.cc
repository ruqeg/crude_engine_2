#include <engine/core/profiler.h>

#include <engine/graphics/asynchronous_loader_manager.h>

static void
crude_gfx_asynchronous_loader_manager_pinned_task_asynchronous_loader_loop_
(
  _In_ void                                               *args
);

void
crude_gfx_asynchronous_loader_manager_intiailize
(
  _In_ crude_gfx_asynchronous_loader_manager                *manager,
  _In_ crude_task_sheduler                                  *task_sheduler,
  _In_ uint64                                                active_async_loaders_max_count
)
{
  manager->async_loaders_valid = true;
  manager->async_loaders_count = 0;
  manager->active_async_loaders_max_count = active_async_loaders_max_count;
  
  crude_task_sheduler_add_pinned_task( task_sheduler, crude_gfx_asynchronous_loader_manager_pinned_task_asynchronous_loader_loop_, manager, active_async_loaders_max_count );
  
  mtx_init( &manager->task_mutex, mtx_plain );
}

void
crude_gfx_asynchronous_loader_manager_add_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
)
{
  manager->async_loaders[ manager->async_loaders_count ] = async_loader;
  ++manager->async_loaders_count;
}

void
crude_gfx_asynchronous_loader_manager_remove_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
)
{
  if ( manager->async_loaders_count == 1 )
  {
    --manager->async_loaders_count;
    goto cleaup;
  }

  for ( uint32 i = 0; i < manager->async_loaders_count; ++i )
  {
    if ( manager->async_loaders[ i ] == async_loader )
    {
      /* simple delswap */
      manager->async_loaders[ i ] = manager->async_loaders[ manager->async_loaders_count - 1 ];
      --manager->async_loaders_count;
      goto cleaup;
    }
  }

cleaup:
  return;
}

void
crude_gfx_asynchronous_loader_manager_deintiailize
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager
)
{
  manager->async_loaders_valid = false;
  mtx_destroy( &manager->task_mutex );
}

static void
crude_gfx_asynchronous_loader_manager_pinned_task_asynchronous_loader_loop_
(
  _In_ void                                               *args
)
{
  crude_gfx_asynchronous_loader_manager *ctx = CRUDE_REINTERPRET_CAST( crude_gfx_asynchronous_loader_manager*, args );
  
  CRUDE_PROFILER_SET_THREAD_NAME( "crude_gfx_asynchronous_loader_manager_pinned_task_asynchronous_loader_loop_" );

  while ( ctx->async_loaders_valid )
  {
    mtx_lock( &ctx->task_mutex );
    for ( uint64 i = 0; i < ctx->async_loaders_count; ++i )
    {
      crude_gfx_asynchronous_loader_update( ctx->async_loaders[ i ] );
    }
    mtx_unlock( &ctx->task_mutex );
  }
}