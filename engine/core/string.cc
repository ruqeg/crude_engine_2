#include <stb_sprintf.h>
#include <string.h>

#include <core/assert.h>
#include <core/string.h>

/************************************************
 *
 * String Buffer
 * 
 ***********************************************/
void
crude_string_buffer_initialize
(
  _Out_ crude_string_buffer                               *string_buffer,
  _In_ size_t                                              capacity,
  _In_ crude_allocator_container                           allocator_container
)
{
  string_buffer->allocator_container = allocator_container;
  string_buffer->buffer = CRUDE_REINTERPRET_CAST( char*, CRUDE_ALLOCATE( allocator_container, capacity + 1 ) );
  string_buffer->buffer[ 0 ] = 0;
  string_buffer->capacity = capacity;
  string_buffer->occupied = 0;
}

void
crude_string_buffer_deinitialize
(
  _In_ crude_string_buffer                                *string_buffer
)
{
  CRUDE_DEALLOCATE( string_buffer->allocator_container, string_buffer->buffer );
  string_buffer->occupied = string_buffer->capacity = 0;
}

char*
crude_string_buffer_append_use_f
(
  _In_ crude_string_buffer                                *string_buffer,
  _In_ char const                                         *format,
  _In_ ...
)
{
  va_list                                                  args;
  int32                                                    written_chars, cached_offset;

  cached_offset = string_buffer->occupied;
  if ( string_buffer->occupied >= string_buffer->capacity )
  {
    CRUDE_OVERFLOW();
    return NULL;
  }

  va_start( args, format );
  written_chars = crude_vsnprintf( &string_buffer->buffer[ string_buffer->occupied ], string_buffer->capacity - string_buffer->occupied, format, args );
  va_end( args );

  if ( written_chars < 0 )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_CORE, "New string too big for current buffer! Please allocate more size." );
  }

  string_buffer->occupied += written_chars > 0 ? written_chars : 0;
  string_buffer->buffer[ string_buffer->occupied ] = 0;
  ++string_buffer->occupied;
  
  return string_buffer->buffer + cached_offset;
}

void
crude_string_buffer_clear
(
  _In_ crude_string_buffer                                *string_buffer
)
{
  string_buffer->occupied = 0;
  string_buffer->buffer[ 0 ] = 0;
}

char*
crude_string_buffer_current
(
  _In_ crude_string_buffer                                *string_buffer
)
{
  return string_buffer->buffer + string_buffer->occupied;
}

void
crude_string_buffer_close_current_string
(
  _In_ crude_string_buffer                                *string_buffer
)
{
  string_buffer->buffer[ string_buffer->occupied ] = 0;
  ++string_buffer->occupied;
}

/************************************************
 *
 * String Utils
 * 
 ***********************************************/
void
crude_snprintf
(
  _Out_ char                                              *buffer,
  _In_ int                                                 buffer_size,
  _In_ char const                                         *format,
  ...
)
{
  va_list args;
  va_start( args, format );
  crude_vsnprintf( buffer, buffer_size, format, args );
  va_end( args );
}
  
int32
crude_vsnprintf
(
  _Out_ char                                              *buffer,
  _In_ int                                                 buffer_size,
  _In_ char const                                         *format,
  va_list                                                  args
)
{
  return stbsp_vsnprintf( buffer, buffer_size, format, args );
}

void
crude_string_copy
(
  _Out_ char                                              *dst,
  _In_ char const                                         *src,
  _In_ size_t                                              n
)
{
  strncpy( dst, src, n );
}

size_t
crude_string_length
(
  _In_ char const                                         *s
)
{
  return strlen( s );
}

CRUDE_API size_t
crude_string_cmp
(
  _In_ char const                                         *s1,
  _In_ char const                                         *s2
)
{
  return strcmp( s1, s2 );
}

void
crude_string_cat
(
  _In_ char                                               *dst,
  _In_ char const                                         *src,
  _In_ size_t                                              n
)
{
  strcat_s( dst, n, src );
}