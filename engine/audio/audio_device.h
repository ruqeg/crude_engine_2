#pragma once

#include <miniaudio.h>

#include <engine/core/alias.h>

typedef struct crude_audio_device
{
  ma_context                                               lma_context;
  ma_device                                                lma_device;
  ma_engine                                                lma_engine;
} crude_audio_device;

CRUDE_API void
crude_audio_device_initialize
(
  _In_ crude_audio_device                                  *audio
);

CRUDE_API void
crude_audio_device_deinitialize
(
  _In_ crude_audio_device                                  *audio
);