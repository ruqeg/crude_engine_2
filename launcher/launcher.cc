#define CR_HOST
#include <cr/cr.h>
#include <filesystem>

#include <core/profiler.h>
#include <engine.h>
#include <sandbox.h>

std::filesystem::path get_executable_path()
{
#ifdef _WIN32
  wchar_t path[ MAX_PATH ] = { 0 };
  GetModuleFileNameW( NULL, path, MAX_PATH );
  return std::filesystem::path( path ).remove_filename();
#else
  return std::filesystem::canonical("/proc/self/exe");
#endif
}

#define PLUGIN_PATH_STR( name ) get_executable_path().concat( CR_PLUGIN( name ) ).string()

int main()
{
  crude_engine                                             engine;
  crude_sandbox                                            sandbox;
  cr_plugin                                                crude_sandbox_cr, crude_engine_simulation_cr;
  
  crude_engine_initialize( &engine, 1u );
  crude_sandbox_initialize( &sandbox, &engine );
  
  crude_sandbox_cr.userdata           = &sandbox;
  crude_engine_simulation_cr.userdata = &engine;
  cr_plugin_open( crude_engine_simulation_cr, PLUGIN_PATH_STR( "crude_engine_simulation" ).c_str() );
  cr_plugin_open( crude_sandbox_cr, PLUGIN_PATH_STR( "crude_sandbox" ).c_str() );

  while ( engine.running )
  {
    cr_plugin_update( crude_sandbox_cr );
    cr_plugin_update( crude_engine_simulation_cr );
    CRUDE_PROFILER_MARK_FRAME;
  }

  cr_plugin_close( crude_engine_simulation_cr );
  cr_plugin_close( crude_sandbox_cr );
  
  crude_sandbox_deinitialize( &sandbox );
  crude_engine_deinitialize( &engine );

  return EXIT_SUCCESS;
}
