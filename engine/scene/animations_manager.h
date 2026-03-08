#pragma once

#include <engine/core/ecs.h>
#include <engine/core/math.h>

typedef struct crude_gfx_model_renderer_resources_manager crude_gfx_model_renderer_resources_manager;

typedef enum crude_animation_channel_path
{
  CRUDE_ANIMATION_CHANNEL_PATH_TRANSLATION,
  CRUDE_ANIMATION_CHANNEL_PATH_ROTATION,
  CRUDE_ANIMATION_CHANNEL_PATH_SCALE
} crude_animation_channel_path;

typedef enum crude_animation_sampler_interpolation_type
{
  CRUDE_ANIMATION_SAMPLER_INTERPOLATION_TYPE_LINEAR
} crude_animation_sampler_interpolation_type;

typedef struct crude_animation_sampler
{
  crude_animation_sampler_interpolation_type               interpolation;
  float32                                                 *inputs;
  XMFLOAT4                                                *outputs;
} crude_animation_sampler;

typedef struct crude_animation_channel
{
  uint32                                                   sampler_index;
  crude_animation_channel_path                             path;
  uint64                                                   node;
} crude_animation_channel;

typedef struct crude_animation
{
  float32                                                  current_time;
  float32                                                  start;
  float32                                                  end;
  crude_animation_channel                                 *channels;
  crude_animation_sampler                                 *samplers;
  char                                                     name[ CRUDE_ANIMATION_NAME_LENGTH_MAX ];
  bool                                                     active;
} crude_animation;

typedef struct crude_animations_manager
{
  crude_gfx_model_renderer_resources_manager              *model_renderer_resources_manager;
  crude_animation                                         *animations;
  crude_heap_allocator                                    *allocator;
} crude_animations_manager;

CRUDE_API void
crude_animations_manager_initialize
(
  _In_ crude_animations_manager                           *manager
);

CRUDE_API void
crude_animations_manager_deinitialize
(
  _In_ crude_animations_manager                           *manager
);

CRUDE_API void
crude_animations_manager_update
(
  _In_ crude_animations_manager                           *manager,
  _In_ crude_ecs                                          *world,
  _In_ float32                                             delta_time
);

CRUDE_API void
crude_animations_manager_add_animation_from_cgltf
(
  _In_ crude_animations_manager                           *manager,
  _In_ void                                               *gltf_raw,
  _In_ void                                               *gltf_animation_raw,
  _In_ uint64                                              nodes_offset
);