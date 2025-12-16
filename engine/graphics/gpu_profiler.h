#pragma once

#if CRUDE_GPU_PROFILER

#include <engine/core/color.h>
#include <engine/graphics/gpu_device.h>

typedef enum crude_gfx_gpu_pipeline_statistics_v
{
  CRUDE_GFX_GPU_PIPELINE_STATISTICS_VERTICES_COUNT,
  CRUDE_GFX_GPU_PIPELINE_STATISTICS_PRIMITIVE_COUNT,
  CRUDE_GFX_GPU_PIPELINE_STATISTICS_VERTEX_SHADER_INVOCATIONS,
  CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_INVOCATIONS,
  CRUDE_GFX_GPU_PIPELINE_STATISTICS_CLIPPING_PRIMITIVES,
  CRUDE_GFX_GPU_PIPELINE_STATISTICS_FRAGMENT_SHADER_INVOCATIONS,
  CRUDE_GFX_GPU_PIPELINE_STATISTICS_COMPUTE_SHADER_INVOCATIONS,
  CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT
};

typedef struct crude_gfx_gpu_time_query
{
  float64                                                  elapsed_ms;
  uint16                                                   start_query_index;
  uint16                                                   end_query_index;
  uint16                                                   parent_index;
  uint16                                                   depth;
  crude_color                                              color;
  uint32                                                   frame_index;
  char const                                              *name;
} crude_gfx_gpu_time_query;

typedef struct crude_gfx_gpu_time_query_tree
{
  crude_gfx_gpu_time_query                                *time_queries;
  uint32                                                   time_queries_count;
  uint16                                                   current_time_query;
  uint16                                                   allocated_time_query;
  uint16                                                   depth;
} crude_gfx_gpu_time_query_tree;

typedef struct crude_gfx_gpu_pipeline_statistics
{
  uint64                                                   statistics[ CRUDE_GFX_GPU_PIPELINE_STATISTICS_COUNT ];
} crude_gfx_gpu_pipeline_statistics;

typedef struct crude_gfx_gpu_time_queries_manager
{
  crude_gfx_gpu_time_query_tree                           *query_trees;
  crude_allocator_container                                allocator_container;
  crude_gfx_gpu_thread_frame_pools                        *thread_frame_pools;
  crude_gfx_gpu_time_query                                *timestamps;
  crude_gfx_gpu_pipeline_statistics                        frame_pipeline_statistics;
  uint32                                                   queries_per_thread;
  uint32                                                   queries_per_frame;
  uint32                                                   num_threads;
  bool                                                     current_frame_resolved;
} crude_gfx_gpu_time_queries_manager;

CRUDE_API void
crude_gfx_gpu_time_queries_manager_initialize
(
  _In_ crude_gfx_gpu_time_queries_manager                 *manager,
  _In_ crude_gfx_gpu_thread_frame_pools                   *thread_frame_pools,
  _In_ crude_allocator_container                           allocator_container,
  _In_ uint16                                              queries_per_thread,
  _In_ uint16                                              num_threads,
  _In_ uint16                                              max_frames
);

CRUDE_API void
crude_gfx_gpu_time_queries_manager_deinitialize
(
  _In_ crude_gfx_gpu_time_queries_manager                 *manager
);

CRUDE_API void
crude_gfx_gpu_time_queries_manager_reset
(
  _In_ crude_gfx_gpu_time_queries_manager                 *manager
);

/* Returns the total queries for this frame. */
CRUDE_API uint32
crude_gfx_gpu_time_queries_manager_resolve
(
  _In_ crude_gfx_gpu_time_queries_manager                 *manager,
  _In_ uint32                                              current_frame,
  _In_ crude_gfx_gpu_time_query                           *timestamps_to_fill
);

CRUDE_API void
crude_gfx_gpu_time_query_tree_reset
(
  _In_ crude_gfx_gpu_time_query_tree                      *time_query_tree
);

CRUDE_API void
crude_gfx_gpu_time_query_tree_set_queries
(
  _In_ crude_gfx_gpu_time_query_tree                      *time_query_tree,
  _In_ crude_gfx_gpu_time_query                           *time_queries,
  _In_ uint32                                              count
);

CRUDE_API crude_gfx_gpu_time_query*
crude_gfx_gpu_time_query_tree_push
(
  _In_ crude_gfx_gpu_time_query_tree                      *time_query_tree,
  _In_ char const                                         *name
);

CRUDE_API crude_gfx_gpu_time_query*
crude_gfx_gpu_time_query_tree_pop
(
  _In_ crude_gfx_gpu_time_query_tree                      *time_query_tree
);

CRUDE_API void
crude_gfx_gpu_pipeline_statistics_reset
(
  _In_ crude_gfx_gpu_pipeline_statistics                  *statistics
);

#endif /* CRUDE_GPU_PROFILER */