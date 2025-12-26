#pragma once

#include <engine/core/alias.h>
#include <engine/core/memory.h>
#include <engine/core/string.h>

typedef struct crude_environment
{
  struct
  {
    char const                                            *render_graph_absolute_directory;
    char const                                            *resources_absolute_directory;
    char const                                            *techniques_absolute_directory;
    char const                                            *shaders_absolute_directory;
    char const                                            *compiled_shaders_absolute_directory;
  } directories;
  struct
  {
    char                                                   initial_title[ 512 ];
    uint64                                                 initial_width;
    uint64                                                 initial_height;
  } window;
  crude_string_buffer                                      constant_string_buffer;
} crude_environment;

CRUDE_API void
crude_environment_initialize
(
  _In_ crude_environment                                  *environment,
  _In_ char const                                         *absolute_filepath,
  _In_ char const                                         *working_absolute_directory,
  _In_ crude_heap_allocator                               *heap_allocator,
  _In_ crude_stack_allocator                              *temporary_allocator
);

CRUDE_API void
crude_environment_deinitialize
(
  _In_ crude_environment                                  *environment
);