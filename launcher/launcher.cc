#define CR_HOST
#include <cr/cr.h>
#include <filesystem>

#include <engine.h>

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
  cr_plugin crude_sandbox, crude_editor, crude_paprika, crude_engine_simulation;
  
  crude_engine engine = crude_engine_initialize( 1u );
  
  crude_sandbox.userdata           = engine.world;
  crude_editor.userdata            = engine.world;
  crude_engine_simulation.userdata = &engine;
  crude_paprika.userdata           = engine.world;
  //cr_plugin_open( crude_editor, CR_PLUGIN( "crude_editor" ) );
  cr_plugin_open( crude_engine_simulation, PLUGIN_PATH_STR( "crude_engine_simulation" ).c_str() );
  cr_plugin_open( crude_sandbox, PLUGIN_PATH_STR( "crude_sandbox" ).c_str() );
  cr_plugin_open( crude_paprika, PLUGIN_PATH_STR( "crude_paprika" ).c_str() );

  while ( engine.running )
  {
    cr_plugin_update( crude_sandbox );
    cr_plugin_update( crude_engine_simulation );
    cr_plugin_update( crude_paprika );
    //cr_plugin_update( crude_editor );
  }

  //cr_plugin_close( crude_editor );
  cr_plugin_close( crude_engine_simulation );
  cr_plugin_close( crude_paprika );
  cr_plugin_close( crude_sandbox );

  crude_engine_deinitialize( &engine );

  return EXIT_SUCCESS;
}
