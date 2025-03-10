#ifdef _WIN32
#include <stb_sprintf.h>
#endif
#include <string.h>

#include <core/string.h>

void
crude_strcat
(
  _Out_ char       *dst_buffer,
  _In_ char        *src_buffer
)
{
  strcat( dst_buffer, src_buffer );
}

void
crude_snprintf
(
  _Out_ char       *buffer,
  _In_ int          buffer_size,
  _In_ char const  *format,
  ...
)
{
  va_list args;
  va_start(args, format);
  crude_vsnprintf( buffer, buffer_size, format, args );
  va_end(args);
}
  
void
crude_vsnprintf
(
  _Out_ char      *buffer,
  _In_ int         buffer_size,
  _In_ char const *format,
  va_list          args
)
{
#ifdef _WIN32
  stbsp_vsnprintf( buffer, buffer_size, format, args );
#endif
}