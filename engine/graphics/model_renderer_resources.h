#pragma once

#include <engine/graphics/gpu_device.h>
#include <engine/graphics/shaders/common/scene.h>

typedef struct crude_transform crude_transform;
typedef struct crude_gfx_model_renderer_resources_manager crude_gfx_model_renderer_resources_manager;

typedef struct crude_gfx_model_renderer_resources_handle
{
  uint32                                                   index;
} crude_gfx_model_renderer_resources_handle;

typedef struct crude_gfx_mesh_cpu
{
  XMFLOAT4                                                 bounding_sphere;
  crude_gfx_memory_allocation                              index_hga;
  crude_gfx_memory_allocation                              position_hga;
  crude_gfx_memory_allocation                              tangent_hga;
  crude_gfx_memory_allocation                              normal_hga;
  crude_gfx_memory_allocation                              texcoord_hga;
  uint32                                                   index_offset;
  uint32                                                   position_offset;
  uint32                                                   tangent_offset;
  uint32                                                   normal_offset;
  uint32                                                   texcoord_offset;
  uint32                                                   indices_count;
  XMFLOAT4                                                 albedo_color_factor;
  XMFLOAT3                                                 metallic_roughness_occlusion_factor;
  float32                                                  alpha_cutoff;
  uint32                                                   flags;
  crude_gfx_texture_handle                                 albedo_texture_handle;
  crude_gfx_texture_handle                                 metallic_roughness_texture_handle;
  crude_gfx_texture_handle                                 normal_texture_handle;
  crude_gfx_texture_handle                                 occlusion_texture_handle;
  uint32                                                   gpu_mesh_index;
  uint32                                                   meshlets_offset;
  uint32                                                   meshlets_count;
} crude_gfx_mesh_cpu;

typedef struct crude_gfx_node
{
  int64                                                    skin;
  int64                                                    parent;
  int64                                                   *childrens;
  int64                                                   *meshes_gpu;
  char                                                     name[ CRUDE_GFX_NODE_NAME_LENGTH_MAX ];
} crude_gfx_node;

typedef enum crude_gfx_animation_channel_path
{
  CRUDE_GFX_ANIMATION_CHANNEL_PATH_TRANSLATION,
  CRUDE_GFX_ANIMATION_CHANNEL_PATH_ROTATION,
  CRUDE_GFX_ANIMATION_CHANNEL_PATH_SCALE
} crude_gfx_animation_channel_path;

typedef enum crude_gfx_animation_sampler_interpolation_type
{
  CRUDE_GFX_ANIMATION_SAMPLER_INTERPOLATION_TYPE_LINEAR
} crude_gfx_animation_sampler_interpolation_type;

typedef struct crude_gfx_animation_sampler
{
  crude_gfx_animation_sampler_interpolation_type           interpolation;
  float32                                                 *inputs;
  XMFLOAT4                                                *outputs;
} crude_gfx_animation_sampler;

typedef struct crude_gfx_animation_channel
{
  uint32                                                   sampler_index;
  crude_gfx_animation_channel_path                         path;
  uint64                                                   node;
} crude_gfx_animation_channel;

typedef struct crude_gfx_animation
{
  float32                                                  start;
  float32                                                  end;
  crude_gfx_animation_channel                             *channels;
  crude_gfx_animation_sampler                             *samplers;
  char                                                     name[ CRUDE_GFX_ANIMATION_NAME_LENGTH_MAX ];
} crude_gfx_animation;

typedef struct crude_gfx_skin
{
  XMFLOAT4X4                                              *inverse_bind_matrices;
  int64                                                   *joints;
} crude_gfx_skin;

typedef struct crude_gfx_model_renderer_resources
{
  crude_gfx_skin                                          *skins;
  crude_gfx_animation                                     *animations;
  crude_gfx_node                                          *nodes;
  crude_transform                                         *default_nodes_transforms;
  crude_transform                                         *animated_nodes_transforms;
  char                                                     relative_filepath[ CRUDE_GFX_MODEL_RESOURCE_RELATIVE_FILEPATH_LENGTH_MAX ];
} crude_gfx_model_renderer_resources;

typedef enum crude_gfx_model_renderer_resoruces_instances_type
{
  CRUDE_GFX_MODEL_RENDERER_RESOURCES_INSTANCE_TYPE_GLTF,
#if CRUDE_DEVELOP
  CRUDE_GFX_MODEL_RENDERER_RESOURCES_INSTANCE_TYPE_DUBUG_GLTF,
  CRUDE_GFX_MODEL_RENDERER_RESOURCES_INSTANCE_TYPE_DUBUG_COLLISION,
#endif
} crude_gfx_model_renderer_resoruces_instances_type;

typedef struct crude_gfx_model_renderer_resources_animation_instance
{
  MOVED NODES TO HERE!
  int64                                                    animation_index;
  float32                                                  current_time;
  bool                                                     inverse;
  bool                                                     loop;
} crude_gfx_model_renderer_resources_animation_instance;

typedef struct crude_gfx_model_renderer_resources_instance
{
  crude_gfx_model_renderer_resources_animation_instance    animation_instance;
  crude_gfx_model_renderer_resources_handle                model_renderer_resources_handle;
  XMFLOAT4X4                                               model_to_world;
  crude_gfx_model_renderer_resoruces_instances_type        type;
} crude_gfx_model_renderer_resources_instance;

CRUDE_API void
crude_gfx_mesh_cpu_to_mesh_draw_gpu
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_draw                               *mesh_draw_gpu
);

XMMATRIX
crude_gfx_node_to_world
(
  _In_ crude_gfx_node const                               *nodes,
  _In_ crude_transform const                              *transforms,
  _In_ uint64                                              node_index
);

CRUDE_API void
crude_gfx_model_renderer_resources_initialize
(
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources
);

CRUDE_API void
crude_gfx_model_renderer_resources_deinitialize
(
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources
);

CRUDE_API crude_gfx_model_renderer_resources_instance
crude_gfx_model_renderer_resources_instance_empty
(
);


CRUDE_API void
crude_gfx_model_renderer_resources_instance_update_animation
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ float32                                             delta_time
);