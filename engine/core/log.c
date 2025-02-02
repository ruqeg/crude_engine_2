#include <core/log.h>
#include <core/string.h>
#include <stdio.h>
#include <Windows.h>

void* g_allocator = NULL;

CRUDE_INLINE cstring crude_get_verbosity_string( crude_verbosity v )
{
  switch ( v )
  {
  case CRUDE_VERBOSITY_OFF:      return "Off";
  case CRUDE_VERBOSITY_ERROR:    return "Error";
  case CRUDE_VERBOSITY_WARNING:  return "Warning";
  case CRUDE_VERBOSITY_INFO:     return "Info";
  case CRUDE_VERBOSITY_ALL:      return "All";
  }
  return "Unknown";
}

CRUDE_INLINE cstring crude_get_channel_string( crude_channel c )
{
  switch ( c )
  {
  case CRUDE_CHANNEL_GENERAL:     return "General";
  case CRUDE_CHANNEL_MEMORY:      return "Memory";
  case CRUDE_CHANNEL_NETWORKING:  return "Networking";
  case CRUDE_CHANNEL_GRAPHICS:    return "Graphics";
  case CRUDE_CHANNEL_COLLISIONS:  return "Collisions";
  case CRUDE_CHANNEL_GAMEPLAY:    return "Gameplay";
  case CRUDE_CHANNEL_SOUND:       return "Sound";
  case CRUDE_CHANNEL_FILEIO:      return "File I/O";
  case CRUDE_CHANNEL_GUI:         return "GUI";
  case CRUDE_CHANNEL_ALL:         return "All";
  }
  return "Unknown-Channel";
}

void crude_log_set_allocator( void* allocator )
{
  g_allocator = allocator;
}

void crude_log_common( cstring filename, int32 line, crude_channel channel, crude_verbosity verbosity, cstring format, ... )
{
  va_list args;
  va_start( args, format );
  crude_string_buffer format_buffer;
  crude_string_buffer message_buffer;
  crude_string_buffer_initialize( &format_buffer, 1024, g_allocator  );
  crude_string_buffer_initialize( &message_buffer, 1024, g_allocator  );
  crude_string_buffer_append_f( &format_buffer, "%s%s%s", "[c: %s][v: %s][f: %s][l: %i] =>\n\t", format, "\n" );
  crude_string_buffer_append_f( &message_buffer, format_buffer.data, crude_get_channel_string( channel ), crude_get_verbosity_string( verbosity ), filename, line, args );
  OutputDebugStringA( CRUDE_CAST( LPCSTR, message_buffer.data ) );
  crude_string_buffer_deinitialize( &format_buffer );
  crude_string_buffer_deinitialize( &format_buffer );
  va_end(args);
}