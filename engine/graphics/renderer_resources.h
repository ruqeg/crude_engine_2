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

typedef struct crude_gfx_renderer_material_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_material_handle;

typedef struct crude_gfx_renderer_technique_handle
{
  crude_gfx_resource_index                                 index;
} crude_gfx_renderer_technique_handle;

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

typedef struct crude_gfx_renderer_technique_pass_creation
{
  crude_gfx_pipeline_handle                                pipeline;
} crude_gfx_renderer_technique_pass_creation;

typedef struct crude_gfx_renderer_technique_creation
{
  char const                                              *json_name;
  crude_gfx_renderer_technique_pass_creation               passes[ 8 ];
  uint32                                                   passes_count;
  char const                                              *name;
} crude_gfx_renderer_technique_creation;

typedef struct crude_gfx_renderer_technique_pass
{
  crude_gfx_pipeline_handle                                pipeline;
  struct { uint64 key; uint16 value; }                    *name_hashed_to_descriptor_index;
} crude_gfx_renderer_technique_pass;

typedef struct crude_gfx_renderer_technique
{
  char const                                              *json_name;
  char const                                              *name;
  crude_gfx_renderer_technique_pass                       *passes;
  uint32                                                   pool_index;
  struct { uint64 key; uint16 value; }                    *name_hashed_to_pass_index;
} crude_gfx_renderer_technique;

typedef struct crude_gfx_renderer_material_creation
{
  crude_gfx_renderer_technique                            *technique;
  char const                                              *name;
  uint32                                                   render_index;
} crude_gfx_renderer_material_creation;

typedef struct crude_gfx_renderer_material
{
  crude_gfx_renderer_technique                            *technique;
  uint32                                                   render_index;
  uint32                                                   pool_index;
  char const                                              *name;
} crude_gfx_renderer_material;

CRUDE_API uint32
crude_gfx_renderer_technique_get_pass_index
(
  _In_ crude_gfx_renderer_technique                       *technique,
  _In_ char const                                         *name
);

CRUDE_API uint16
crude_gfx_renderer_technique_pass_get_binding_index
(
  _In_ crude_gfx_renderer_technique_pass                  *technique_pass,
  _In_ char const                                         *name
);

CRUDE_API void
crude_gfx_renderer_technique_creation_add_pass
(
  _In_ crude_gfx_renderer_technique_creation              *creation,
  _In_ crude_gfx_pipeline_handle                           pipeline
);