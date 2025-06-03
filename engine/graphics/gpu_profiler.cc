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
}

void
crude_gfx_gpu_time_queries_manager_deinitialize
(
  _In_ crude_gfx_gpu_time_queries_manager                 *manager
)
{
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
  return 0;
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