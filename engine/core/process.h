#pragma once

#include <core/alias.h>

CRUDE_API bool
crude_process_execute
(
  _In_ char const                                         *working_directory,
  _In_ char const                                         *process_fullpath,
  _In_ char const                                         *arguments,
  _In_ char const                                         *search_error_string
);

CRUDE_API void
crude_process_expand_environment_strings
(
  _In_ char const                                         *environment,
  _In_ char                                               *buffer,
  _In_ uint32                                              buffer_size
);

CRUDE_API bool
crude_shell_execute
(
  _In_ char const                                         *filepath
);