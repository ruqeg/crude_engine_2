#pragma once

#include <graphics/gpu_resources.h>
#include <core/memory.h>

typedef uint32                                             crude_gfx_render_graph_handle;

typedef struct crude_gfx_render_graph_resource_handle
{
  crude_gfx_render_graph_handle                            index;
} crude_gfx_render_graph_resource_handle;

typedef struct crude_gfx_render_graph_node_handle
{
  crude_gfx_render_graph_handle                            index;
} crude_gfx_render_graph_node_handle;

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

typedef struct crude_gfx_render_graph_resoruce
{
  crude_gfx_render_graph_resource_type                     type;
  crude_gfx_render_graph_resource_info                     resource_info;
  crude_gfx_render_graph_node_handle                       producer;
  crude_gfx_render_graph_resource_handle                   output_handle;
  int32                                                    ref_count;
  char const                                              *name;
} crude_gfx_render_graph_resoruce;

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

typedef struct crude_gfx_render_graph_node_creation
{
  CRUDE_ARR( crude_gfx_render_graph_resource_input_creation )  inputs;
  CRUDE_ARR( crude_gfx_render_graph_resource_output_creation ) outputs;
  bool                                                     enabled;
  char const                                              *name;
} crude_gfx_render_graph_node_creation;

typedef struct crude_gfx_render_graph_node
{
  int32                                                    ref_count;
  crude_gfx_render_pass_handle                             render_pass;
  crude_gfx_framebuffer_handle                             framebuffer;
  //crude_gfx_render_graph_render_pass                      *graph_render_pass;
  CRUDE_ARR( crude_gfx_render_graph_resource_handle )      inputs;
  CRUDE_ARR( crude_gfx_render_graph_resource_handle )      outputs;
  CRUDE_ARR( crude_gfx_render_graph_node_handle )          edges;
  bool                                                     enabled;
  char const                                              *name;
} crude_gfx_render_graph_node;

typedef struct crude_gfx_render_graph
{
  crude_allocator local_allocator;
} crude_gfx_render_graph;

CRUDE_API void
crude_gfx_render_graph_parse_from_file
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ char const                                         *file_path,
  _In_ crude_allocator                                     temp_allocator
);