#include <flecs.h>

#define CR_HOST
#include <cr/cr.h>
#include <filesystem>

#include <engine.h>
#include <core/profiler.h>
#include <sandbox.h>
#include <paprika.h>

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
  crude_paprika                                            paprika;
  cr_plugin                                                crude_sandbox_cr, crude_paprika_cr, crude_engine_simulation_cr;

  crude_engine_initialize( &engine, 1u );
  crude_sandbox_initialize( &sandbox, &engine );
  crude_paprika_initialize( &paprika, &engine );

  crude_sandbox_cr.userdata           = &sandbox;
  crude_paprika_cr.userdata           = &paprika;
  crude_engine_simulation_cr.userdata = &engine;

  cr_plugin_open( crude_engine_simulation_cr, PLUGIN_PATH_STR( "crude_engine_simulation" ).c_str() );
  cr_plugin_open( crude_sandbox_cr, PLUGIN_PATH_STR( "crude_sandbox" ).c_str() );
  cr_plugin_open( crude_paprika_cr, PLUGIN_PATH_STR( "crude_paprika" ).c_str() );

  while ( engine.running )
  {
    if ( sandbox.working )
    {
      cr_plugin_update( crude_sandbox_cr );
    }
    if ( paprika.working )
    {
      cr_plugin_update( crude_paprika_cr );
    }
    cr_plugin_update( crude_engine_simulation_cr );
    CRUDE_PROFILER_MARK_FRAME;
  }
  
  cr_plugin_close( crude_paprika_cr );
  cr_plugin_close( crude_sandbox_cr );
  cr_plugin_close( crude_engine_simulation_cr );
  
  crude_paprika_deinitialize( &paprika );
  crude_sandbox_deinitialize( &sandbox );
  crude_engine_deinitialize( &engine );

  return EXIT_SUCCESS;
}
