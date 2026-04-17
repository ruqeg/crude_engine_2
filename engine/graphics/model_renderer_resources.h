#pragma once

#include <engine/graphics/gpu_device.h>
#include <engine/graphics/shaders/common/scene.h>

typedef struct crude_transform crude_transform;
typedef struct crude_gfx_model_renderer_resources_manager crude_gfx_model_renderer_resources_manager;

typedef enum crude_gfx_animation_channel_path
{
  CRUDE_GFX_ANIMATION_CHANNEL_PATH_TRANSLATION,
  CRUDE_GFX_ANIMATION_CHANNEL_PATH_ROTATION,
  CRUDE_GFX_ANIMATION_CHANNEL_PATH_SCALE
} crude_gfx_animation_channel_path;

typedef enum crude_gfx_animation_sampler_interpolation_type
{
  CRUDE_GFX_ANIMATION_SAMPLER_INTERPOLATION_TYPE_LINEAR,
  CRUDE_GFX_ANIMATION_SAMPLER_INTERPOLATION_TYPE_STEP
} crude_gfx_animation_sampler_interpolation_type;

typedef struct crude_gfx_model_renderer_resources_handle
{
  int64                                                    index;
} crude_gfx_model_renderer_resources_handle;

typedef struct crude_gfx_aabb_cpu
{
  XMFLOAT3                                                 max;
  XMFLOAT3                                                 min;
} crude_gfx_aabb_cpu;

typedef struct crude_gfx_mesh_cpu
{
  crude_gfx_aabb_cpu                                      *affected_joints_local_aabb;
  uint32                                                  *affected_joints; /* need to calculate animation bounding box, i would like to move all this bullshit to the compute shader, but naaah im too lazy */
  XMFLOAT4                                                 default_bounding_sphere;
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
  XMFLOAT4                                                 emmision;
  XMFLOAT4                                                 albedo_color_factor;
  XMFLOAT3                                                 metallic_roughness_occlusion_factor;
  float32                                                  alpha_cutoff;
  uint32                                                   flags;
  crude_gfx_texture_handle                                 albedo_texture_handle;
  crude_gfx_texture_handle                                 metallic_roughness_texture_handle;
  crude_gfx_texture_handle                                 normal_texture_handle;
  crude_gfx_texture_handle                                 occlusion_texture_handle;
  uint32                                                   meshlets_offset;
  uint32                                                   meshlets_count;
  uint64                                                   gpu_mesh_global_index;
} crude_gfx_mesh_cpu;

typedef struct crude_gfx_node
{
  int64                                                    skin;
  int64                                                    parent;
  int64                                                   *childrens;
  uint64                                                  *meshes;
  char                                                     name[ CRUDE_GFX_NODE_NAME_LENGTH_MAX ];
} crude_gfx_node;

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
  crude_gfx_mesh_cpu                                      *meshes;
  char                                                     relative_filepath[ CRUDE_GFX_MODEL_RESOURCE_RELATIVE_FILEPATH_LENGTH_MAX ];
  CRUDE_HASHMAPSTR( uint64 )                              *animation_name_to_index;  
  VkAccelerationStructureKHR                              *vk_blases;
  crude_gfx_memory_allocation                             *blases_hga;
} crude_gfx_model_renderer_resources;

typedef struct crude_gfx_model_renderer_resources_animation_instance
{
  int64                                                    animation_index;
  float32                                                  current_time;
  bool                                                     inverse;
  bool                                                     paused;
  bool                                                     disabled;
  float32                                                  speed;
  bool                                                     loop;
  int32                                                    nodes_enabled_bits[ 8 ];
  crude_transform                                         *nodes_transforms;
} crude_gfx_model_renderer_resources_animation_instance;

typedef struct crude_gfx_model_renderer_resources_instance
{
  crude_transform                                         *nodes_transforms;
  crude_gfx_model_renderer_resources_animation_instance    animations_instances[ 8 ];
  crude_gfx_model_renderer_resources_handle                model_renderer_resources_handle;
  XMFLOAT4X4                                               model_to_world;
  bool                                                     cast_shadow;
  uint64                                                   unique_id;
} crude_gfx_model_renderer_resources_instance;

CRUDE_API void
crude_gfx_mesh_cpu_to_mesh_draw_gpu
(
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_gfx_mesh_cpu const                           *mesh,
  _Out_ crude_gfx_mesh_draw                               *mesh_draw_gpu
);

CRUDE_API XMMATRIX
crude_gfx_node_to_model
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

CRUDE_API void
crude_gfx_model_renderer_resources_instance_initialize
(
  _In_ crude_gfx_model_renderer_resources_instance        *instance,
  _In_opt_ crude_gfx_model_renderer_resources_manager     *manager,
  _In_opt_ crude_gfx_model_renderer_resources_handle       handle
);

CRUDE_API void
crude_gfx_model_renderer_resources_instance_deinitialize
(
  _In_ crude_gfx_model_renderer_resources_instance        *instance
);

CRUDE_API void
crude_gfx_model_renderer_resources_update_instance_animations
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ float32                                             delta_time
);

CRUDE_API void
crude_gfx_model_renderer_resources_instance_blend_one_animation
(
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ int64                                               animation_instance_index
);

CRUDE_API void
crude_gfx_model_renderer_resources_instance_blend_two_animations
(
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ uint32                                              from_index,
  _In_ uint32                                              to_index,
  _In_ float32                                             blend_factor
);

CRUDE_API void
crude_gfx_model_renderer_resources_instance_blend_animations
(
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ int64                                               animations_indices[ 8 ],
  _In_ float32                                             weights[ 8 ]
);

CRUDE_API bool
crude_gfx_model_renderer_resources_animation_instance_is_enabled_node
(
  _Inout_ crude_gfx_model_renderer_resources_animation_instance *animation_instance,
  _In_ int64                                               node_index
);

CRUDE_API void
crude_gfx_model_renderer_resources_animation_instance_enable_node
(
  _Inout_ crude_gfx_model_renderer_resources_animation_instance *animation_instance,
  _In_ int64                                               node_index,
  _In_ bool                                                enabled
);

CRUDE_API int64
crude_gfx_model_renderer_resources_instance_find_node_by_name
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Inout_ crude_gfx_model_renderer_resources_instance     *model_renderer_resources_instance,
  _In_ char const                                         *name
);

CRUDE_API uint64
crude_gfx_model_renderer_resources_instance_find_animation_index_by_name
(
  _In_ crude_gfx_model_renderer_resources_instance        *instance,
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ char const                                         *name
);