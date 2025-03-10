#pragma once

#include <core/alias.h>

CRUDE_API void
crude_strcat
(
  _Out_ char       *dst_buffer,
  _In_ char        *src_buffer
);

CRUDE_API void
crude_snprintf
(
  _Out_ char       *buffer,
  _In_ int          buffer_size,
  _In_ char const  *format,
  ...
);
  
CRUDE_API void
crude_vsnprintf
(
  _Out_ char      *buffer,
  _In_ int         buffer_size,
  _In_ char const *format,
  va_list          args
);