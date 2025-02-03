#pragma once

#include <core/alias.h>
#include <scene/world.h>

typedef void ( *crude_update_callback )( ecs_iter_t *it );

typedef struct crude_gui_system
{
  crude_update_callback update;
} crude_gui_system;

typedef struct crude_gui_context
{
  void *value;
} crude_gui_context;

typedef struct crude_window
{
  int32 width;
  int32 height;
  bool  maximized;
} crude_window;

typedef struct crude_window_handle
{
  void *value;
} crude_window_handle;

extern ECS_COMPONENT_DECLARE( crude_gui_system );
extern ECS_COMPONENT_DECLARE( crude_gui_context );
extern ECS_COMPONENT_DECLARE( crude_window );
extern ECS_COMPONENT_DECLARE( crude_window_handle );

void crude_gui_components_import( crude_world *world );