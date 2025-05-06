#pragma once

#include <graphics/gpu_device.h>
#include <core/memory.h>

/************************************************
 *
 * Render Graph Handles
 * 
 ***********************************************/
typedef uint32                                             crude_gfx_render_graph_handle;

typedef struct crude_gfx_render_graph_resource_handle
{
  crude_gfx_render_graph_handle                            index;
} crude_gfx_render_graph_resource_handle;

typedef struct crude_gfx_render_graph_node_handle
{
  crude_gfx_render_graph_handle                            index;
} crude_gfx_render_graph_node_handle;

/************************************************
 *
 * Render Graph Constants
 * 
 ***********************************************/

#define CRUDE_GFX_RENDER_GRAPH_RESOURCE_HANDLE_INVALID ( ( crude_gfx_render_graph_resource_handle ) { CRUDE_RESOURCE_INDEX_INVALID } )
#define CRUDE_GFX_RENDER_GRAPH_NODE_HANDLE_INVALID ( ( crude_gfx_render_graph_node_handle ) { CRUDE_RESOURCE_INDEX_INVALID } )

#define CRUDE_GFX_RENDER_GRAPH_MAX_RENDER_PASS_COUNT 256
#define CRUDE_GFX_RENDER_GRAPH_MAX_RESOURCES_COUNT   1024
#define CRUDE_GFX_RENDER_GRAPH_MAX_NODES_COUNT       1024

/************************************************
 *
 * Render Graph Resource
 * 
 ***********************************************/

typedef enum crude_gfx_render_graph_resource_type
{
  CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_INVALID    = -1,
  CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_BUFFER     = 0,
  CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE    = 1,
  CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT = 2,
  CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_REFERENCE  = 3
} crude_gfx_render_graph_resource_type;

typedef struct crude_gfx_render_graph_resource_info
{
  bool                                                     external;
  
  union
  {
    struct
    {
      sizet                                                size;
      VkBufferUsageFlags                                   flags;
      crude_gfx_buffer_handle                              buffer;
    } buffer;
    struct
    {
      uint32                                               width;
      uint32                                               height;
      uint32                                               depth;
      VkFormat                                             format;
      VkImageUsageFlags                                    flags;
      crude_gfx_render_pass_operation                      load_op;
      crude_gfx_texture_handle                             texture;
    } texture;
  };
} crude_gfx_render_graph_resource_info;

typedef struct crude_gfx_render_graph_resource
{
  crude_gfx_render_graph_resource_type                     type;
  crude_gfx_render_graph_resource_info                     resource_info;
  crude_gfx_render_graph_node_handle                       producer;
  crude_gfx_render_graph_resource_handle                   output_handle;
  int32                                                    ref_count;
  char const                                              *name;
} crude_gfx_render_graph_resource;

typedef struct crude_gfx_render_graph_resource_input_creation
{
  crude_gfx_render_graph_resource_type                     type;
  crude_gfx_render_graph_resource_info                     resource_info;
  char const                                              *name;
} crude_gfx_render_graph_resource_input_creation;

typedef struct crude_gfx_render_graph_resource_output_creation
{
  crude_gfx_render_graph_resource_type                     type;
  crude_gfx_render_graph_resource_info                     resource_info;
  char const                                              *name;
} crude_gfx_render_graph_resource_output_creation;

/************************************************
 *
 * Render Graph Nodes
 * 
 ***********************************************/

typedef void (*crude_gfx_render_graph_render_pass_pre_render)( void *ctx, crude_gfx_cmd_buffer *gpu_commands );
typedef void (*crude_gfx_render_graph_render_pass_render)( void *ctx, crude_gfx_cmd_buffer *gpu_commands );
typedef void (*crude_gfx_render_graph_render_pass_on_resize)( void *ctx, crude_gfx_device *gpu, uint32 new_width, uint32 new_height );

typedef struct crude_gfx_render_graph_pass
{
  crude_gfx_render_graph_render_pass_pre_render            pre_render;
  crude_gfx_render_graph_render_pass_render                render;
  crude_gfx_render_graph_render_pass_on_resize             on_resize;
  void                                                    *ctx;
} crude_gfx_render_graph_pass;

typedef struct crude_gfx_render_graph_node_creation
{
  crude_gfx_render_graph_resource_input_creation          *inputs;
  crude_gfx_render_graph_resource_output_creation         *outputs;
  bool                                                     enabled;
  char const                                              *name;
} crude_gfx_render_graph_node_creation;

typedef struct crude_gfx_render_graph_node
{
  int32                                                    ref_count;
  crude_gfx_render_pass_handle                             render_pass;
  crude_gfx_framebuffer_handle                             framebuffer;
  crude_gfx_render_graph_pass                             *graph_render_pass;
  crude_gfx_render_graph_resource_handle                  *inputs;
  crude_gfx_render_graph_resource_handle                  *outputs;
  crude_gfx_render_graph_node_handle                      *edges;
  bool                                                     enabled;
  char const                                              *name;
} crude_gfx_render_graph_node;

/************************************************
 *
 * Render Graph Builder Structs
 * 
 ***********************************************/
typedef struct crude_gfx_render_graph_node_cache
{
  struct { uint64 key; uint32 value; }                    *node_map;
  crude_resource_pool                                      nodes;
} crude_gfx_render_graph_node_cache;

typedef struct crude_gfx_render_graph_resource_cache
{
  struct { uint64 key; uint32 value; }                    *resource_map;
  crude_resource_pool                                      resources;
} crude_gfx_render_graph_resource_cache;

typedef struct crude_gfx_render_graph_pass_cache
{
  struct { uint64 key; crude_gfx_render_graph_pass *value; } *render_pass_map;
} crude_gfx_render_graph_pass_cache;

typedef struct crude_gfx_render_graph_builder
{
  crude_gfx_device                                        *gpu;
  crude_allocator_container                                allocator_container;
  crude_gfx_render_graph_node_cache                        node_cache;
  crude_gfx_render_graph_resource_cache                    resource_cache;
  crude_gfx_render_graph_pass_cache                        render_pass_cache;
} crude_gfx_render_graph_builder;

/************************************************
 *
 * Render Graph Structs
 * 
 ***********************************************/
typedef struct crude_gfx_render_graph
{
  crude_gfx_render_graph_node_handle                      *nodes;
  crude_gfx_render_graph_builder                          *builder;
  crude_linear_allocator                                   local_allocator;
  crude_allocator_container                                local_allocator_container;
  char const                                              *name;
} crude_gfx_render_graph;

/************************************************
 *
 * Render Graph Functions Declaration
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_render_graph_initialize
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_gfx_render_graph_builder                     *builder
);

CRUDE_API void
crude_gfx_render_graph_deinitialize
(
  _In_ crude_gfx_render_graph                             *render_graph
);

CRUDE_API void
crude_gfx_render_graph_parse_from_file
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ char const                                         *file_path,
  _In_ crude_stack_allocator                              *temp_allocator
);

CRUDE_API void
crude_gfx_render_graph_compile
(
  _In_ crude_gfx_render_graph                             *render_graph
);

CRUDE_API void
crude_gfx_render_graph_render
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_gfx_cmd_buffer                               *gpu_commands
);

/************************************************
 *
 * Render Graph Builder Function Declaration
 * 
 ***********************************************/
CRUDE_API void
crude_gfx_render_graph_builder_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_device                                   *gpu
);

CRUDE_API void
crude_gfx_render_graph_builder_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
);

CRUDE_API void
crude_gfx_render_graph_builder_register_render_pass
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ char const                                         *name,
  _In_ crude_gfx_render_graph_pass                        *render_pass
);

CRUDE_API crude_gfx_render_graph_node_handle
crude_gfx_render_graph_builder_create_node
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_render_graph_node_creation const         *creation
);

CRUDE_API crude_gfx_render_graph_resource_handle
crude_gfx_render_graph_builder_create_node_output
(
  _In_ crude_gfx_render_graph_builder                        *builder,
  _In_ crude_gfx_render_graph_resource_output_creation const *creation,
  _In_ crude_gfx_render_graph_node_handle                     producer
);

CRUDE_API crude_gfx_render_graph_resource_handle
crude_gfx_render_graph_builder_create_node_input
(
  _In_ crude_gfx_render_graph_builder                       *builder,
  _In_ crude_gfx_render_graph_resource_input_creation const *creation
);

CRUDE_API crude_gfx_render_graph_node*
crude_gfx_render_graph_builder_access_node
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_render_graph_node_handle                  handle
);

CRUDE_API crude_gfx_render_graph_node*
crude_gfx_render_graph_builder_access_node_by_name
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ char const                                         *name
);

CRUDE_API crude_gfx_render_graph_node_handle
crude_gfx_render_graph_builder_obtain_node
(
  _In_ crude_gfx_render_graph_builder                     *builder
);

CRUDE_API crude_gfx_render_graph_resource_handle
crude_gfx_render_graph_builder_obtain_resource
(
  _In_ crude_gfx_render_graph_builder                     *builder
);

CRUDE_API crude_gfx_render_graph_resource*
crude_gfx_render_graph_builder_access_resource
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_render_graph_resource_handle              handle
);

CRUDE_API crude_gfx_render_graph_resource*
crude_gfx_render_graph_builder_access_resource_by_name
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ char const                                         *name
);

CRUDE_API void
crude_gfx_render_graph_builder_resource_cache_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
);

CRUDE_API void
crude_gfx_render_graph_builder_node_cache_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
);

CRUDE_API void
crude_gfx_render_graph_builder_render_pass_cache_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
);

CRUDE_API void
crude_gfx_render_graph_builder_resource_cache_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
);

CRUDE_API void
crude_gfx_render_graph_builder_node_cache_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
);

CRUDE_API void
crude_gfx_render_graph_builder_render_pass_cache_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
);