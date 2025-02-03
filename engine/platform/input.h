#pragma once

#include <scene/world.h>

typedef bool ( *crude_input_event_callback )( const void * );
typedef struct crude_input
{
  crude_input_event_callback callback;
} crude_input;

extern ECS_COMPONENT_DECLARE( crude_input );

void crude_input_components_import( crude_world *world );