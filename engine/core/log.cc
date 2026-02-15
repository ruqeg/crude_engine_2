#ifdef _WIN32
#include <Windows.h>
#include <stdio.h>
#endif

#include <threads.h>

#include <engine/core/file.h>
#include <engine/core/string.h>
#include <engine/core/assert.h>

#include <engine/core/core_config.h>
#include <engine/core/log.h>

char                                                      *loged_buffer_;
char                                                       message_buffer_[ CRUDE_CORE_LOG_MESSAGE_BUFFER_MAX_LENGTH ];
char                                                       format_buffer_[ CRUDE_CORE_LOG_FORMAT_BUFFER_MAX_LENGTH ];
uint64                                                     loged_bytes_ = 0;
FILE                                                      *log_file_ = NULL;
mtx_t                                                      log_mutex_;

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
    case CRUDE_CHANNEL_AUDIO:       return "Audio";
    case CRUDE_CHANNEL_FILEIO:      return "File I/O";
    case CRUDE_CHANNEL_GUI:         return "GUI";
    case CRUDE_CHANNEL_ALL:         return "All";
  }
  return "Unknown-Channel";
}

void
crude_log_initialize
(
)
{
  loged_buffer_ = CRUDE_CAST( char*, malloc( CRUDE_RMEGA( 1 ) ) );
  log_file_ = fopen( "crude_log.txt", "w" );
  mtx_init( &log_mutex_, mtx_plain );
}

void
crude_log_deinitialize
(
)
{
  free( loged_buffer_ );
  mtx_destroy( &log_mutex_ );
  fclose( log_file_ );
}

char const*
crude_log_buffer
(
)
{
  return loged_buffer_;
}


uint64
crude_log_buffer_length
(
)
{
  return loged_bytes_;
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
  crude_log_common_va( filename, line, channel, verbosity, format, args );
  va_end( args );
}

void
crude_log_common_va
(
  _In_ char const                                         *filename,
  _In_ int32                                               line,
  _In_ crude_channel                                       channel,
  _In_ crude_verbosity                                     verbosity,
  _In_ char const                                         *format,
  _In_ va_list                                             args
)
{
  int32                                                    message_length;

  mtx_lock( &log_mutex_ );

  crude_snprintf( format_buffer_, CRUDE_COUNTOF( message_buffer_ ), "[ %s ][ %s ][ %s ][ line: %i ]\n\t=> %s\n\n", get_verbosity_string_( verbosity ), get_channel_string_( channel ), filename, line, format );
  message_length = crude_vsnprintf( message_buffer_, CRUDE_COUNTOF( message_buffer_ ), format_buffer_, args );

#ifdef _WIN32
  OutputDebugStringA( ( LPCSTR )message_buffer_ );
#endif

  printf( "%s", message_buffer_ );
  
  if ( log_file_ )
  {
    fprintf( log_file_, message_buffer_ );
    if ( verbosity == CRUDE_VERBOSITY_ERROR || verbosity == CRUDE_VERBOSITY_WARNING )
    {
      fflush( log_file_ );
    }
  }

  for ( uint32 i = 0; i < message_length; ++i )
  {
    if ( loged_bytes_ + i > CRUDE_RMEGA( 1 ) - 1 )
    {
      break;
    }
    loged_buffer_[ loged_bytes_ + i ] = message_buffer_[ i ]; 
  }

  loged_bytes_ += message_length;

  if ( loged_bytes_ < CRUDE_RMEGA( 1 ) )
  {
    loged_buffer_[ loged_bytes_ ] = 0;
  }
  else
  {
    loged_buffer_[ CRUDE_RMEGA( 1 ) - 1 ] = 0;
  }

  if ( loged_bytes_ > CRUDE_RMEGA( 1 ) )
  {
    if ( log_file_ )
    {
      fseek( log_file_, 0, SEEK_SET );  
      fprintf( log_file_, "\n=== LOG ROTATED AT %zu bytes ===\n\n", loged_bytes_ );
    }

    system("clear || cls");
    printf( "\n=== LOG ROTATED AT %zu bytes ===\n\n", loged_bytes_ );

    loged_bytes_ = 0;
  }

  mtx_unlock( &log_mutex_ );
}