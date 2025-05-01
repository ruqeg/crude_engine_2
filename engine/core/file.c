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


static long _get_file_size
(
  _In_ FILE                                               *f
)
{
  fseek( f, 0, SEEK_END );
  long file_size_signed = ftell( f );
  fseek( f, 0, SEEK_SET );
  return file_size_signed;
}

void
crude_get_current_working_directory
(
  _Out_ char                                              *buffer,
  _In_ uint32                                              buffer_size
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
  _In_ char                                               *path
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
  _Out_ char                                              *path
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

bool
crude_file_exist
(
  _In_ char                                               *path
)
{
#if defined( _WIN64 )
  WIN32_FILE_ATTRIBUTE_DATA unused;
  return GetFileAttributesExA( path, GetFileExInfoStandard, &unused );
#else
  int result = access( path, F_OK );
  return ( result == 0 );
#endif
}

CRUDE_API bool
crude_read_file
(
  _In_ char const                                         *filename,
  _In_ crude_allocator_container                           allocator,
  _Out_ uint8                                            **buffer,
  _Out_ uint32                                            *buffer_size
)
{
  FILE* file = fopen( filename, "r" );
  
  if ( !file )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_FILEIO, "Cannor read file \"%s\"", filename );
    return false;
  }

  sizet filesize = _get_file_size( file );
  *buffer = CRUDE_REALLOCATE( allocator, *buffer, filesize + 1 );
  *buffer_size = fread( *buffer, 1, filesize, file );
  (*buffer)[ *buffer_size ] = 0;
  fclose( file );
  return true;
}