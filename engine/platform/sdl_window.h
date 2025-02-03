#pragma once

#include <scene/world.h>
#include <core/alias.h>

typedef struct crude_sdl_window
{
  uint32  dummy;
} crude_sdl_window;

extern ECS_COMPONENT_DECLARE( crude_sdl_window );

void crude_sdl_components_import( crude_world *world );
