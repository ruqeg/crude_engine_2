#include <core/log.h>
#include <stb_sprintf.h>
#include <Windows.h>

char g_message_buffer[1024];
char g_format_buffer[1024];

static CRUDE_INLINE char const* crude_get_verbosity_string( _In_ crude_verbosity v )
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

static CRUDE_INLINE char const* crude_get_channel_string( _In_ crude_channel c )
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

static CRUDE_INLINE void output_visual_studio( _In_ char* buffer )
{
  OutputDebugStringA( CAST( LPCSTR, buffer ) );
}

void crude_log_common( _In_ char const *filename, _In_ int32 line, _In_ crude_channel channel, _In_ crude_verbosity verbosity, _In_ char const *format, ... )
{
  va_list args;
  va_start( args, format );
  stbsp_snprintf( g_format_buffer, ARRAY_SIZE( g_message_buffer ), "[c: %s][v: %s][f: %s][l: %i] =>\n\t%s\n", crude_get_channel_string( channel ), crude_get_verbosity_string( verbosity ), filename, line, format );
  stbsp_vsnprintf( g_message_buffer, ARRAY_SIZE( g_message_buffer ), g_format_buffer, args );
  output_visual_studio( g_message_buffer );
  va_end(args);
}