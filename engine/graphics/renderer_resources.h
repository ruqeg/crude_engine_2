#pragma once

#include <graphics/gpu_resources.h>

/************************************************
 *
 * Invalid Renderer Resoruces Indices
 * 
 ***********************************************/
#define CRUDE_GFX_RENDERER_TEXTURE_INDEX_INVALID ( ~0u )

/************************************************
 *
 * Renderer Resoruces Handles
 * 
 ***********************************************/
typedef struct crude_gfx_renderer_buffer_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_buffer_handle;

typedef struct crude_gfx_renderer_texture_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_texture_handle;

typedef struct crude_gfx_renderer_sampler_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_sampler_handle;

typedef struct crude_gfx_renderer_program_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_program_handle;

typedef struct crude_gfx_renderer_material_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_material_handle;

/************************************************
 *
 * Renderer Resoruces Structs
 * 
 ***********************************************/
typedef struct crude_gfx_renderer_buffer
{
  uint32                                                   references;
  crude_gfx_buffer_handle                                  handle;
  uint32                                                   pool_index;
  crude_gfx_buffer_description                             desc;
  char const                                              *name;
} crude_gfx_renderer_buffer;

typedef struct crude_gfx_renderer_texture
{
  uint32                                                   references;
  crude_gfx_texture_handle                                 handle;
  uint32                                                   pool_index;
  char const                                              *name;
} crude_gfx_renderer_texture;

typedef struct crude_gfx_renderer_sampler
{
  uint32                                                   references;
  crude_gfx_sampler_handle                                 handle;
  uint32                                                   pool_index;
  char const                                              *name;
} crude_gfx_renderer_sampler;

typedef struct crude_gfx_renderer_program_creation
{
  crude_gfx_pipeline_creation                              pipeline_creation;
} crude_gfx_renderer_program_creation;

typedef struct crude_gfx_renderer_program_pass
{
  crude_gfx_pipeline_handle                                pipeline;
  crude_gfx_descriptor_set_layout_handle                   descriptor_set_layout;
} crude_gfx_renderer_program_pass;

typedef struct crude_gfx_renderer_program
{
  crude_gfx_renderer_program_pass                          passes[ 1 ];
  uint32                                                   pool_index;
  char const                                              *name;
} crude_gfx_renderer_program;

typedef struct crude_gfx_renderer_material_creation
{
  crude_gfx_renderer_program                              *program;
  char const                                              *name;
  uint32                                                   render_index;
} crude_gfx_renderer_material_creation;

typedef struct crude_gfx_renderer_material
{
  crude_gfx_renderer_program                              *program;
  uint32                                                   render_index;
  uint32                                                   pool_index;
  char const                                              *name;
} crude_gfx_renderer_material;