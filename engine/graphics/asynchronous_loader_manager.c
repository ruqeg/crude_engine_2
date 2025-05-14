#include <TaskScheduler_c.h>

#include <core/profiler.h>

#include <graphics/asynchronous_loader_manager.h>

static void
pinned_task_asynchronous_loader_loop_
(
  _In_ crude_gfx_asynchronous_loader_manager              *ctx
)
{
  CRUDE_PROFILER_SET_THREAD_NAME( "AsynchronousLoaderThread" );

  while ( ctx->async_loaders_valid )
  {
    mtx_lock( &ctx->task_mutex );
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( ctx->async_loaders ); ++i )
    {
      crude_gfx_asynchronous_loader_update( ctx->async_loaders[ i ] );
    }
    mtx_unlock( &ctx->task_mutex );
  }
}

void
crude_gfx_async_loader_task_manager_intiailize
(
  _In_ crude_gfx_asynchronous_loader_manager                *manager,
  _In_ crude_gfx_asynchronous_loader_manager_creation const *creation
)
{
  struct enkiTaskSchedulerConfig config = enkiGetTaskSchedulerConfig( creation->task_sheduler );
  manager->task_sheduler = creation->task_sheduler;
  manager->async_loaders_valid = true;
  manager->async_loader_task = enkiCreatePinnedTask( creation->task_sheduler, pinned_task_asynchronous_loader_loop_, config.numTaskThreadsToCreate );
  enkiAddPinnedTaskArgs( creation->task_sheduler, manager->async_loader_task, manager );
  mtx_init( &manager->task_mutex, mtx_plain );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( manager->async_loaders, config.numTaskThreadsToCreate, creation->allocator_container );
}

void
crude_gfx_async_loader_task_manager_add_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
)
{
  CRUDE_ARRAY_PUSH( manager->async_loaders, async_loader );
}

void
crude_gfx_async_loader_task_manager_remove_loader
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager,
  _In_ crude_gfx_asynchronous_loader                      *async_loader
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->async_loaders ); ++i )
  {
    if ( manager->async_loaders[ i ] == async_loader )
    {
      CRUDE_ARRAY_DELSWAP( manager->async_loaders, i );
    }
  }
}

void
crude_gfx_async_loader_task_manager_deintiailize
(
  _In_ crude_gfx_asynchronous_loader_manager              *manager
)
{
  manager->async_loaders_valid = false;
  CRUDE_ARRAY_DEINITIALIZE( manager->async_loaders );
  mtx_destroy( &manager->task_mutex );
}