#pragma once

#include <miniaudio.h>

#include <engine/core/alias.h>
#include <engine/core/resource_pool.h>

typedef struct crude_sound_handle
{
  uint32                                                   index;
} crude_sound_handle;

typedef struct crude_sound_group_handle
{
  uint32                                                   index;
} crude_sound_group_handle;

#define CRUDE_SOUND_GROUP_HANDLE_INVALID                   ( CRUDE_COMPOUNT( crude_sound_group_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )
#define CRUDE_SOUND_HANDLE_INVALID                         ( CRUDE_COMPOUNT( crude_sound_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )

typedef enum crude_audio_sound_positioning
{
  CRUDE_AUDIO_SOUND_POSITIONING_ABSOLUTE = ma_positioning_absolute,
  CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE = ma_positioning_relative
} crude_audio_sound_positioning;

typedef enum crude_audio_sound_attenuation_model
{
  CRUDE_AUDIO_SOUND_ATTENUATION_MODEL_NONE = ma_attenuation_model_none,
  CRUDE_AUDIO_SOUND_ATTENUATION_MODEL_INVERSE = ma_attenuation_model_inverse,
  CRUDE_AUDIO_SOUND_ATTENUATION_MODEL_LINEAR = ma_attenuation_model_linear,
  CRUDE_AUDIO_SOUND_ATTENUATION_MODEL_EXPONENTIAL = ma_attenuation_model_exponential
} crude_audio_sound_attenuation_model;

typedef struct crude_sound_creation
{
  char const                                              *absolute_filepath;
  bool                                                     looping;
  bool                                                     async_loading;
  bool                                                     stream;
  bool                                                     decode;
  crude_audio_sound_positioning                            positioning;
  crude_audio_sound_attenuation_model                      attenuation_model;
  float32                                                  max_distance;
  float32                                                  min_distance;
  float32                                                  rolloff;
  crude_sound_group_handle                                 sound_group_handle;
} crude_sound_creation;

CRUDE_API crude_sound_creation
crude_sound_creation_empty
(
);