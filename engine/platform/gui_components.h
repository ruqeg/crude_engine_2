#pragma once

#include <flecs.h>

#include <core/alias.h>

typedef void ( *crude_update_callback )( ecs_iter_t* );

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

CRUDE_API extern ECS_COMPONENT_DECLARE( crude_gui_system );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_gui_context );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_window );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_window_handle );

CRUDE_API void crude_gui_componentsImport( ecs_world_t *world );