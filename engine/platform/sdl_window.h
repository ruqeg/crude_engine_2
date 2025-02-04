#pragma once

#include <scene/world.h>
#include <core/alias.h>

typedef struct crude_sdl_window
{
  uint32  dummy;
} crude_sdl_window;

CRUDE_API extern ECS_COMPONENT_DECLARE( crude_sdl_window );

CRUDE_API void crude_sdl_componentsImport( crude_world *world );
