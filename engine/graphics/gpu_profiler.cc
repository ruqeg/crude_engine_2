#include <graphics/gpu_profiler.h>

void
crude_gfx_gpu_time_queries_manager_initialize
(
  _In_ crude_gfx_gpu_time_queries_manager                 *manager,
  _In_ crude_gfx_gpu_thread_frame_pools                   *thread_frame_pools,
  _In_ crude_allocator_container                           allocator_container,
  _In_ uint16                                              queries_per_thread,
  _In_ uint16                                              num_threads,
  _In_ uint16                                              max_frames
)
{
  manager->num_threads = num_threads;
  manager->thread_frame_pools = thread_frame_pools;
  manager->queries_per_thread = queries_per_thread;
  manager->queries_per_frame = queries_per_thread * num_threads;
  manager->allocator_container = allocator_container;

  uint32 total_time_queries = manager->queries_per_frame * max_frames;
  uint64 allocated_size = sizeof( crude_gfx_gpu_time_query ) * total_time_queries;
  uint8 *memory = CRUDE_CAST( uint8*, CRUDE_ALLOCATE( allocator_container, allocated_size ) );

  manager->timestamps = ( crude_gfx_gpu_time_query* )memory;
  memset( manager->timestamps, 0, sizeof( crude_gfx_gpu_time_query ) * total_time_queries );

  uint32 num_pools = num_threads * max_frames;

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( manager->query_trees, num_pools, allocator_container );

  for ( uint32 i = 0; i < num_pools; ++i )
  {
    crude_gfx_gpu_time_query_tree *query_tree = &manager->query_trees[ i ];
    crude_gfx_gpu_time_query_tree_set_queries( query_tree, &manager->timestamps[i * queries_per_thread], queries_per_thread );
  }

  crude_gfx_gpu_time_queries_manager_reset( manager );
}

void
crude_gfx_gpu_time_queries_manager_deinitialize
(
  _In_ crude_gfx_gpu_time_queries_manager                 *manager
)
{
  CRUDE_DEALLOCATE( manager->allocator_container, manager->timestamps );
  CRUDE_ARRAY_DEINITIALIZE( manager->query_trees );
}

void
crude_gfx_gpu_time_queries_manager_reset
(
  _In_ crude_gfx_gpu_time_queries_manager                 *manager
)
{
}

uint32
crude_gfx_gpu_time_queries_manager_resolve
(
  _In_ crude_gfx_gpu_time_queries_manager                 *manager,
  _In_ uint32                                              current_frame,
  _In_ crude_gfx_gpu_time_query                           *timestamps_to_fill
)
{
  uint32 copied_timestamps = 0;
  for ( uint32 t = 0; t < manager->num_threads; ++t )
  {
    uint32 pool_index = ( manager->num_threads * current_frame ) + t;
    crude_gfx_gpu_thread_frame_pools *thread_pools = &manager->thread_frame_pools[ pool_index ];
    crude_gfx_gpu_time_query_tree *time_query = thread_pools->time_queries;
    if ( time_query && time_query->allocated_time_query )
    {
      crude_memory_copy( timestamps_to_fill + copied_timestamps, &manager->timestamps[ pool_index * manager->queries_per_thread ], sizeof( crude_gfx_gpu_time_query ) * time_query->allocated_time_query );
      copied_timestamps += time_query->allocated_time_query;
    }
  }
  return copied_timestamps;
}

void
crude_gfx_gpu_time_query_tree_reset
(
  _In_ crude_gfx_gpu_time_query_tree                      *time_query_tree
)
{
  time_query_tree->current_time_query = 0;
  time_query_tree->allocated_time_query = 0;
  time_query_tree->depth = 0;
}

void
crude_gfx_gpu_time_query_tree_set_queries
(
  _In_ crude_gfx_gpu_time_query_tree                      *time_query_tree,
  _In_ crude_gfx_gpu_time_query                           *time_queries,
  _In_ uint32                                              count
)
{
  time_query_tree->time_queries = time_queries;
  time_query_tree->time_queries_count = count;
  crude_gfx_gpu_time_query_tree_reset( time_query_tree );
}

crude_gfx_gpu_time_query*
crude_gfx_gpu_time_query_tree_push
(
  _In_ crude_gfx_gpu_time_query_tree                      *time_query_tree,
  _In_ char const                                         *name
)
{
  crude_gfx_gpu_time_query *time_query = &time_query_tree->time_queries[ time_query_tree->allocated_time_query ];
  time_query->start_query_index = time_query_tree->allocated_time_query * 2;
  time_query->end_query_index = time_query->start_query_index + 1;
  time_query->depth = time_query_tree->depth++;
  time_query->name = name;
  time_query->parent_index = time_query_tree->current_time_query;
  
  time_query_tree->current_time_query = time_query_tree->allocated_time_query;
  ++time_query_tree->allocated_time_query;
  
  return time_query;
}

crude_gfx_gpu_time_query*
crude_gfx_gpu_time_query_tree_pop
(
  _In_ crude_gfx_gpu_time_query_tree                      *time_query_tree
)
{
  crude_gfx_gpu_time_query *time_query = &time_query_tree->time_queries[ time_query_tree->current_time_query ];
  time_query_tree->current_time_query = time_query->parent_index;
  time_query_tree->depth--;
  return time_query;
}

void
crude_gfx_gpu_pipeline_statistics_reset
(
  _In_ crude_gfx_gpu_pipeline_statistics                  *statistics
)
{
  for ( uint32 i = 0u; i < CRUDE_COUNTOF( statistics->statistics ); ++i )
  {
    statistics->statistics[ i ] = 0u;
  }
}