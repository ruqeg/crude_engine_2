#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#include <core/assert.h>

#include <core/file.h>

void
crude_get_current_working_directory
(
  _Out_ char                                    *buffer,
  _In_ uint32                                    buffer_size
)
{
  CRUDE_ASSERTM( CRUDE_CHANNEL_FILEIO, buffer_size >= FILENAME_MAX, "Working directory buffer size must be larger than %i!", FILENAME_MAX );
#if defined( _WIN64 )
  char* result = GetCurrentDir( buffer, buffer_size );
  CRUDE_ASSERTM( CRUDE_CHANNEL_FILEIO, result, "Failed to get current directory!" );
#endif // _WIN64
}

void
crude_change_working_directory
(
  _In_ char                                     *path
)
{
#if defined( _WIN64 )
  if ( !SetCurrentDirectoryA( path ) )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_FILEIO, "Cannot change current directory to %s\n", path );
  }
#else
  if ( chdir( path ) != 0 )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_FILEIO, "Cannot change current directory to %s\n", path );
  }
#endif // _WIN64
}

void
crude_file_directory_from_path
(
  _Out_ char                                    *path
)
{
  char* last_point = strrchr( path, '.' );
  char* last_separator = strrchr( path, '/' );
  if ( last_separator != NULL && last_point > last_separator )
  {
    *(last_separator + 1) = 0;
  }
  else
  {
    last_separator = strrchr( path, '\\' );
    if ( last_separator != NULL && last_point > last_separator )
    {
      *( last_separator + 1 ) = 0;
    }
    else
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_FILEIO, "Malformed path %s", path );
    }
  }
}