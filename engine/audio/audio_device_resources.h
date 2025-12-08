#pragma once

#include <miniaudio.h>

#include <engine/core/alias.h>

typedef struct crude_sound_handle
{
  uint32                                                   index;
} crude_sound_handle;

#define CRUDE_SOUND_HANDLE_INVALID                         ( CRUDE_COMPOUNT( crude_sound_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )

typedef enum crude_audio_sound_positioning
{
  CRUDE_AUDIO_SOUND_POSITIONING_ABSOLUTE = ma_positioning_absolute,
  CRUDE_AUDIO_SOUND_POSITIONING_RELATIVE = ma_positioning_relative
} crude_audio_sound_positioning;

typedef struct crude_sound_creation
{
  char const                                              *absolute_filepath;
  bool                                                     looping;
  bool                                                     async_loading;
  bool                                                     stream;
  bool                                                     decode;
  crude_audio_sound_positioning                            positioning;
} crude_sound_creation;

CRUDE_API crude_sound_creation
crude_sound_creation_empty
(
);