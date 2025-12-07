#pragma once

#include <engine/core/ecs.h>
#include <engine/audio/audio_device.h>

typedef struct crude_audio_system_context
{
  crude_audio_device                                      *device;
} crude_audio_system_context;

CRUDE_API void
crude_audio_system_import
(
  _In_ ecs_world_t                                        *world,
  _In_ crude_audio_system_context                         *ctx
);