#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>

#include <core/alias.h>

typedef bool ( *crude_input_event_callback )( const void * );

typedef struct crude_key_state
{
  bool                         pressed;
  bool                         state;
  bool                         current;
} crude_key_state;

typedef struct crude_mouse_input
{
  float32                      x;
  float32                      y;
} crude_mouse_input;

typedef struct crude_mouse_state
{
  crude_key_state              left;
  crude_key_state              right;
  crude_mouse_input            wnd;
  crude_mouse_input            rel;
  crude_mouse_input            view;
  crude_mouse_input            scroll;
} crude_mouse_state;

typedef struct crude_input
{
  crude_input_event_callback   callback;
  crude_key_state              keys[ 128 ];
  crude_mouse_state            mouse;
  crude_mouse_input            wrapwnd;
} crude_input;

typedef struct crude_window
{
  int32                  width;
  int32                  height;
  bool                   maximized;
} crude_window;

typedef struct crude_window_handle
{
  void                  *value;
} crude_window_handle;

CRUDE_API extern ECS_COMPONENT_DECLARE( crude_window );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_window_handle );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_input );

CRUDE_API void
crude_platform_componentsImport
(
  ecs_world_t                 *world
);