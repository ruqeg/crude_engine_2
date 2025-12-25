#pragma once

#include <engine/core/ecs.h>
#include <engine/audio/audio_device.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/
typedef struct crude_audio_listener
{
  float32                                                   last_local_to_world_update_time;
} crude_audio_listener;

CRUDE_API ECS_COMPONENT_DECLARE( crude_audio_listener );

CRUDE_API void
crude_audio_components_import
(
  _In_ crude_ecs                                          *world
);

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_audio_system_context
{
  crude_ecs                                               *world;
  crude_audio_device                                      *device;
} crude_audio_system_context;

CRUDE_API void
crude_audio_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_audio_system_context                         *ctx
);