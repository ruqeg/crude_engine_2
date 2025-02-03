#include <core/string.h>
#include <core/assert.h>
#include <core/memory.h>
#include <core/utils.h>
#include <string.h>
#include <stdio.h>

void crude_string_buffer_initialize( crude_string_buffer* string, sizet size, void* allocator )
{
  if ( string->data )
  {
    crude_deallocate( allocator, string->data );
  }

  if ( size < 1)
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_CORE, "Buffer cannot be empty!" );
    return;
  }

  string->allocator = allocator;
  string->data = CRUDE_CAST( char*, crude_allocate( allocator, size, 0u ) );
  string->data[0] = 0u;
  string->buffer_size = size;
  string->current_size = 0u;
}

void crude_string_buffer_deinitialize( crude_string_buffer* string )
{
  crude_deallocate( string->allocator, string->data );
  string->data = NULL;
  string->buffer_size = 0u;
  string->current_size = 0u;
}

void crude_string_buffer_append( crude_string_buffer* string, const char* appended_string )
{
  crude_string_buffer_append_f( string, "%s", appended_string );
}

void crude_string_buffer_append_f( crude_string_buffer* string, const char* format, ... )
{
  if ( string->current_size >= string->buffer_size )
  {
    CRUDE_OVERFLOW();
    return;
  }

  va_list args;
  va_start( args, format );
  int written_chars = vsnprintf_s( &string->data[ string->current_size ], string->buffer_size - string->current_size, _TRUNCATE, format, args );
  string->current_size += written_chars > 0 ? written_chars : 0;
  va_end( args );

  if ( written_chars < 0 )
  {
    CRUDE_OVERFLOW();
    CRUDE_ABORT( CRUDE_CHANNEL_CORE, "New string too big for current buffer! Please allocate more size." );
  }
}
