#pragma once

#include <core/memory.h>

CRUDE_API void
crude_get_current_working_directory
(
  _Out_ char                                              *buffer,
  _In_ uint32                                              buffer_size
);

CRUDE_API void
crude_change_working_directory
(
  _In_ char                                               *path
);

CRUDE_API void
crude_file_directory_from_path
(
  _Out_ char                                              *path
);

CRUDE_API bool
crude_file_exist
(
  _In_ char                                               *path
);

CRUDE_API bool
crude_read_file
(
  _In_ char const                                         *filename,
  _In_ crude_allocator_container                           allocator_container,
  _Out_ uint8                                            **buffer,
  _Out_ uint32                                            *buffer_size
);

CRUDE_API bool
crude_read_file_binary
(
  _In_ char const                                         *filename,
  _In_ crude_allocator_container                           allocator_container,
  _Out_ uint8                                            **buffer,
  _Out_ uint32                                            *buffer_size
);

CRUDE_API void
crude_write_file
(
  _In_ char const                                         *filename,
  _In_ void                                               *buffer,
  _In_ size_t                                              buffer_size
);

CRUDE_API bool
crude_file_delete
(
  _In_ char const                                         *path
);