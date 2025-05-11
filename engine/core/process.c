#include <Windows.h>
#include <stdio.h>

#include <core/log.h>

#include <core/process.h>

#define CRUDE_PROCESS_LOG_BUFFER_SIZE 256
char                s_process_log_buffer[ CRUDE_PROCESS_LOG_BUFFER_SIZE ];
static char         k_process_output_buffer[ 1025 ];

void
win32_get_error_
(
  _In_ char                                               *buffer,
  _In_ uint32                                              size
)
{
  DWORD errorCode = GetLastError();
  
  char *error_string;
  if ( !FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, errorCode, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&error_string, 0, NULL ) )
  {
    return;
  }

  sprintf_s( buffer, size, "%s", error_string );
  LocalFree( error_string );
}

/* From the post in https://stackoverflow.com/questions/35969730/how-to-read-output-from-cmd-exe-using-createprocess-and-createpipe/55718264#55718264 */
bool
crude_process_execute
(
  _In_ char const                                         *working_directory,
  _In_ char const                                         *process_fullpath,
  _In_ char const                                         *arguments,
  _In_ char const                                         *search_error_string
)
{
  PROCESS_INFORMATION                                      process_info;
  HANDLE                                                   handle_stdin_pipe_read, handle_stdin_pipe_write, handle_stdout_pipe_read, handle_std_pipe_write;
  BOOL                                                     ok;
  bool                                                     execution_success;

  {
    SECURITY_ATTRIBUTES                                    security_attributes;
    
    security_attributes = ( SECURITY_ATTRIBUTES ){ sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };
    ok = CreatePipe( &handle_stdin_pipe_read, &handle_stdin_pipe_write, &security_attributes, 0 );
    if ( ok == FALSE )
    {
      return false;
    }
    ok = CreatePipe( &handle_stdout_pipe_read, &handle_std_pipe_write, &security_attributes, 0 );
    if ( ok == FALSE )
    {
      return false;
    }
  }

  {
    STARTUPINFOA                                           startup_info;
    BOOL                                                   inherit_handles;

    startup_info = ( STARTUPINFOA ){ 0 };
    startup_info.cb = sizeof( startup_info );
    startup_info.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startup_info.hStdInput = handle_stdin_pipe_read;
    startup_info.hStdError = handle_std_pipe_write;
    startup_info.hStdOutput = handle_std_pipe_write;
    startup_info.wShowWindow = SW_SHOW;
    
    execution_success = false;
    process_info = ( PROCESS_INFORMATION ){ 0 };
    inherit_handles = TRUE;
    if ( CreateProcessA( process_fullpath, (char*)arguments, 0, 0, inherit_handles, 0, 0, working_directory, &startup_info, &process_info ) )
    {
      CloseHandle( process_info.hThread );
      CloseHandle( process_info.hProcess );
      
      execution_success = true;
    }
    else
    {
      win32_get_error_( &s_process_log_buffer[0], CRUDE_PROCESS_LOG_BUFFER_SIZE );
      
      CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Execute process error.\n Exe: \"%s\" - Args: \"%s\" - Work_dir: \"%s\"", process_fullpath, arguments, working_directory );
      CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Message: %s\n", s_process_log_buffer );
    }
    CloseHandle( handle_stdin_pipe_read );
    CloseHandle( handle_std_pipe_write );
  }

  /* Output */
  DWORD bytes_read;
  ok = ReadFile( handle_stdout_pipe_read, k_process_output_buffer, 1024, &bytes_read, NULL );
  
  /* Consume all outputs.
   * Terminate current read and initialize the next. */
  while ( ok == TRUE )
  {
    k_process_output_buffer[bytes_read] = 0;
    CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "%s", k_process_output_buffer );
    
    ok = ReadFile( handle_stdout_pipe_read, k_process_output_buffer, 1024, &bytes_read, NULL );
  }
  
  if ( strlen(search_error_string) > 0 && strstr( k_process_output_buffer, search_error_string ) )
  {
    execution_success = false;
  }
  
  /* Close handles. */
  CloseHandle( handle_stdout_pipe_read );
  CloseHandle( handle_stdin_pipe_write );
  
  DWORD process_exit_code = 0;
  GetExitCodeProcess( process_info.hProcess, &process_exit_code );
  
  return execution_success;
}

bool
crude_process_expand_environment_strings
(
  _In_ char const                                         *environment,
  _In_ char                                               *buffer,
  _In_ uint32                                              buffer_size
)
{
  ExpandEnvironmentStringsA( environment, buffer, buffer_size );
}