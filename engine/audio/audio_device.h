#pragma once

#include <engine/core/resource_pool.h>
#include <engine/core/math.h>
#include <engine/core/hash_map.h>
#include <engine/audio/audio_device_resources.h>

typedef struct crude_audio_device
{
  ma_context                                               lma_context;
  ma_device                                                lma_device;
  ma_engine                                                lma_engine;
  crude_resource_pool                                      sounds;
  crude_heap_allocator                                    *allocator;
  ma_fence                                                 fence;
} crude_audio_device;

CRUDE_API void
crude_audio_device_initialize
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_heap_allocator                                *allocator
);

CRUDE_API void
crude_audio_device_wait_wait_till_uploaded
(
  _In_ crude_audio_device                                  *audio
);

CRUDE_API crude_sound_handle
crude_audio_device_create_sound
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_creation const                          *creation
);

CRUDE_API void
crude_audio_device_sound_start
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle
);

CRUDE_API void
crude_audio_device_sound_start
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle
);

CRUDE_API void
crude_audio_device_sound_set_translation
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle,
  _In_ XMVECTOR                                             translation
);

CRUDE_API void
crude_audio_device_sound_set_volume
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle,
  _In_ float32                                              volume
);

CRUDE_API void
crude_audio_device_listener_set_local_to_world
(
  _In_ crude_audio_device                                  *audio,
  _In_ XMMATRIX                                             local_to_world
);

CRUDE_API void
crude_audio_device_destroy_sound
(
  _In_ crude_audio_device                                  *audio,
  _In_ crude_sound_handle                                   sound_handle
);

CRUDE_API void
crude_audio_device_deinitialize
(
  _In_ crude_audio_device                                  *audio
);