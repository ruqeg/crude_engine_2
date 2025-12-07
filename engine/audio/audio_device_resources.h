#pragma once

#include <engine/core/alias.h>

typedef struct crude_sound_handle
{
  uint32                                                   index;
} crude_sound_handle;

#define CRUDE_SOUND_HANDLE_INVALID                         ( CRUDE_COMPOUNT( crude_sound_handle, { CRUDE_RESOURCE_INDEX_INVALID } ) )

typedef struct crude_sound_creation
{
  char const                                              *absolute_filepath;
  bool                                                     looping;
} crude_sound_creation;

CRUDE_API crude_sound_creation
crude_sound_creation_empty
(
);