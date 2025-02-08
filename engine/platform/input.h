#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>
#include <stb_ds.h>

#include <core/alias.h>

typedef bool ( *crude_input_event_callback )( const void * );
typedef struct crude_input
{
  crude_input_event_callback callback;
  SDL_Event                  sld_events[8];
} crude_input;

CRUDE_API extern ECS_COMPONENT_DECLARE( crude_input );

CRUDE_API void crude_input_componentsImport( ecs_world_t *world );