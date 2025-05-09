#ifdef _WIN32
#include <Windows.h>
#include <stdio.h>
#endif

#include <core/file.h>
#include <core/string.h>
#include <core/assert.h>

#include <core/log.h>

char g_message_buffer[ 1024 * 8 ];
char g_format_buffer[ 1024 * 8 ];

FILE *g_log_file = NULL;

static CRUDE_INLINE char const*
get_verbosity_string_
(
  _In_ crude_verbosity v
)
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

static CRUDE_INLINE char const*
get_channel_string_
(
  _In_ crude_channel c
)
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

CRUDE_API void
crude_log_initialize
()
{
  g_log_file = fopen( "log.txt", "w" );
}

CRUDE_API void
crude_log_deinitialize
()
{
  fclose( g_log_file );
}

void
crude_log_common
(
  _In_ char const                                         *filename,
  _In_ int32                                               line,
  _In_ crude_channel                                       channel,
  _In_ crude_verbosity                                     verbosity,
  _In_ char const                                         *format,
  _In_ ...
)
{
  va_list args;
  va_start( args, format );
  crude_snprintf( g_format_buffer, ARRAY_SIZE( g_message_buffer ), "[c: %s][v: %s][f: %s][l: %i] =>\n\t%s\n", get_channel_string_( channel ), get_verbosity_string_( verbosity ), filename, line, format );
  crude_vsnprintf( g_message_buffer, ARRAY_SIZE( g_message_buffer ), g_format_buffer, args );
#ifdef _WIN32
  OutputDebugStringA( ( LPCSTR )g_message_buffer );
#endif
  fprintf( g_log_file, g_message_buffer );
  printf( "%s", g_message_buffer );
  va_end( args );
}