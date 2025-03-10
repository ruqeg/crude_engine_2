#include <stdio.h>

#ifdef _WIN32
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
  _Out_ char    *buffer,
  _In_ uint32    buffer_size
)
{
  CRUDE_ASSERTM( CRUDE_CHANNEL_CORE, buffer_size >= FILENAME_MAX, "Working directory buffer size must be larger than %i!", FILENAME_MAX );
#ifdef _WIN32
  char* result = GetCurrentDir( buffer, buffer_size );
  CRUDE_ASSERTM( CRUDE_CHANNEL_CORE, result, "Failed to get current directory!" );
#endif
}
