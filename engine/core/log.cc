#ifdef _WIN32
#include <Windows.h>
#include <stdio.h>
#endif

#include <core/file.h>
#include <core/string.h>
#include <core/assert.h>

#include <core/log.h>

char                                                       message_buffer_[ CRUDE_RKILO( 8 ) ];
char                                                       format_buffer_[ CRUDE_RKILO( 8 ) ];
FILE                                                      *log_file_ = NULL;

static char const*
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
  //log_file_ = fopen( "crude_log.txt", "w" );
}

CRUDE_API void
crude_log_deinitialize
()
{
  //fclose( log_file_ );
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
  crude_snprintf( format_buffer_, CRUDE_COUNTOF( message_buffer_ ), "[ %s ][ %s ][ %s ][ line: %i ]\n\t=> %s\n\n", get_verbosity_string_( verbosity ), get_channel_string_( channel ), filename, line, format );
  crude_vsnprintf( message_buffer_, CRUDE_COUNTOF( message_buffer_ ), format_buffer_, args );
#ifdef _WIN32
  OutputDebugStringA( ( LPCSTR )message_buffer_ );
#endif
  if ( log_file_ )
  {
    fprintf( log_file_, message_buffer_ );
  }
  printf( "%s", message_buffer_ );
  va_end( args );
}