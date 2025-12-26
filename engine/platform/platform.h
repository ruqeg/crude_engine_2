#pragma once

#include <threads.h>

#include <engine/platform/platform_resources.h>

typedef void ( *crude_platform_quit_callback_function )
(
  _In_ void                                               *ctx
);

typedef struct crude_platform_creation
{
  struct 
  {
    char const                                            *title;
    uint64                                                 width;
    uint64                                                 height;
    bool                                                   maximized;
    uint64                                                 flags;
  } window;
  crude_platform_quit_callback_function                    quit_callback;
  void                                                    *quit_callback_ctx;
  crude_input_callback_function                            input_callback;
  void                                                    *input_callback_ctx;
} crude_platform_creation;

typedef struct crude_platform
{
  SDL_Window                                              *sdl_window;
  uint64                                                   width;
  uint64                                                   height;
  uint64                                                   flags;
  bool                                                     maximized;
  crude_input                                              input;
  crude_platform_quit_callback_function                    quit_callback;
  void                                                    *quit_callback_ctx;
} crude_platform;

typedef struct crude_input_thread_data
{
  crude_input                                              input;
  mtx_t                                                    mutex;
} crude_input_thread_data;

CRUDE_API void
crude_input_thread_data_initialize
(
  _In_ crude_input_thread_data                            *thread_data
);

CRUDE_API void
crude_input_thread_data_deinitialize
(
  _In_ crude_input_thread_data                            *thread_data
);

CRUDE_API void
crude_input_thread_data_flush_input
(
  _In_ crude_input_thread_data                            *thread_data,
  _Out_ crude_input                                       *input
);

CRUDE_API void
crude_input_thread_data_affect_by_input
(
  _In_ crude_input_thread_data                            *thread_data,
  _In_ crude_input const                                  *input
);

CRUDE_API void
crude_platform_service_initialize
(
);

CRUDE_API void
crude_platform_service_deinitialize
(
);

CRUDE_API void
crude_platform_intialize
(
  _In_ crude_platform                                     *platform,
  _In_ crude_platform_creation const                      *creation
);

CRUDE_API void
crude_platform_deintialize
(
  _In_ crude_platform                                     *platform
);

CRUDE_API void
crude_platform_update
(
  _In_ crude_platform                                     *platform
);

CRUDE_API void
crude_platform_hide_cursor
(
  _In_ crude_platform                                     *platform
);

CRUDE_API void
crude_platform_show_cursor
(
  _In_ crude_platform                                     *platform
);