#pragma once

#include <SDL3/SDL.h>

#include <engine/core/alias.h>

typedef void ( *crude_input_callback_function )
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

typedef struct crude_key_state
{
  bool                                                     pressed;
  bool                                                     state;
  bool                                                     current;
} crude_key_state;

typedef struct crude_mouse_input
{
  float32                                                  x;
  float32                                                  y;
} crude_mouse_input;

typedef struct crude_mouse_state
{
  crude_key_state                                          left;
  crude_key_state                                          right;
  crude_mouse_input                                        wnd;
  crude_mouse_input                                        rel;
  crude_mouse_input                                        view;
  crude_mouse_input                                        scroll;
} crude_mouse_state;

typedef struct crude_input
{
  crude_key_state                                          keys[ SDL_SCANCODE_COUNT ];
  crude_mouse_state                                        mouse;
  crude_key_state                                          prev_keys[ SDL_SCANCODE_COUNT ];
  crude_mouse_state                                        prev_mouse;
  crude_input_callback_function                            callback;
  void                                                    *ctx;
} crude_input;

CRUDE_API void
crude_platform_key_down
(
  _In_ crude_key_state *key
);

CRUDE_API void
crude_platform_key_up
(
  _In_ crude_key_state *key
);

CRUDE_API void
crude_platform_key_reset
(
  _In_ crude_key_state *key
);

CRUDE_API void
crude_platform_mouse_reset
(
  _In_ crude_mouse_state *state
);