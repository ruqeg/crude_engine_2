#pragma once

#include <core/alias.h>

CRUDE_API void
crude_get_current_working_directory
(
  _Out_ char                                    *buffer,
  _In_ uint32                                    buffer_size
);

CRUDE_API void
crude_change_working_directory
(
  _In_ char                                     *path
);

CRUDE_API void
crude_file_directory_from_path
(
  _Out_ char                                    *path
);