#pragma once

#include <engine/core/memory.h>

/************************************************
 *
 * String Buffer
 * 
 ***********************************************/
typedef struct crude_string_buffer
{
  char                                                    *buffer;
  sizet                                                    capacity;
  sizet                                                    occupied;
  crude_allocator_container                                allocator_container;
} crude_string_buffer;

CRUDE_API void
crude_string_buffer_initialize
(
  _Out_ crude_string_buffer                               *string_buffer,
  _In_ size_t                                              capacity,
  _In_ crude_allocator_container                           allocator_container
);

CRUDE_API void
crude_string_buffer_deinitialize
(
  _In_ crude_string_buffer                                *string_buffer
);

CRUDE_API char*
crude_string_buffer_append_use_f
(
  _In_ crude_string_buffer                                *string_buffer,
  _In_ char const                                         *format,
  _In_ ...
);

CRUDE_API void
crude_string_buffer_append_m
(
  _In_ crude_string_buffer                                *string_buffer,
  _In_ void const                                         *memory,
  _In_ uint32                                              size
);

CRUDE_API void
crude_string_buffer_clear
(
  _In_ crude_string_buffer                                *string_buffer
);

CRUDE_API char*
crude_string_buffer_current
(
  _In_ crude_string_buffer                                *string_buffer
);

CRUDE_API void
crude_string_buffer_close_current_string
(
  _In_ crude_string_buffer                                *string_buffer
);

/************************************************
 *
 * String Utils
 * 
 ***********************************************/
CRUDE_API void
crude_snprintf
(
  _Out_ char                                              *buffer,
  _In_ int                                                 buffer_size,
  _In_ char const                                         *format,
  ...
);
  
CRUDE_API int32
crude_vsnprintf
(
  _Out_ char                                              *buffer,
  _In_ int                                                 buffer_size,
  _In_ char const                                         *format,
  va_list                                                  args
);

CRUDE_API void
crude_string_copy
(
  _Out_ char                                              *dst,
  _In_ char const                                         *src,
  _In_ size_t                                              n
);

CRUDE_API uint64
crude_string_copy_unknow_length
(
  _Out_ char                                              *dst,
  _In_ char const                                         *src,
  _In_ size_t                                              dst_max_size
);

CRUDE_API size_t
crude_string_length
(
  _In_ char const                                         *s
);

CRUDE_API size_t
crude_string_cmp
(
  _In_ char const                                         *s1,
  _In_ char const                                         *s2
);

CRUDE_API void
crude_string_cat
(
  _In_ char                                               *dst,
  _In_ char const                                         *src,
  _In_ size_t                                              n
);