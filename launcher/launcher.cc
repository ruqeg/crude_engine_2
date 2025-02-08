#define CR_HOST
#include <cr.h>
#include <filesystem>
#include <Windows.h>

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

int APIENTRY wWinMain(_In_ HINSTANCE      hInstance,
                      _In_opt_ HINSTANCE  hPrevInstance,
                      _In_ LPWSTR         lpCmdLine,
                      _In_ int            nCmdShow)
{
  cr_plugin crude_sandbox, crude_editor, crude_engine_simulation;
  
  crude_engine engine = crude_engine_initialize( 1u );
  
  crude_sandbox.userdata           = engine.world;
  crude_editor.userdata            = engine.world;
  crude_engine_simulation.userdata = &engine;
  std::filesystem::path a;
  std::filesystem::path b;
  //cr_plugin_open( crude_editor, CR_PLUGIN( "crude_editor" ) );
  cr_plugin_open( crude_engine_simulation, PLUGIN_PATH_STR( "crude_engine_simulation" ).c_str() );
  cr_plugin_open( crude_sandbox, PLUGIN_PATH_STR( "crude_sandbox" ).c_str() );

  while ( engine.running )
  {
    if ( cr_plugin_update( crude_sandbox ) == 0 )
    {
      cr_plugin_update( crude_engine_simulation );
    }
    cr_plat_init();
  }

  //cr_plugin_close( crude_editor );
  cr_plugin_close( crude_engine_simulation );
  cr_plugin_close( crude_sandbox );

  return EXIT_SUCCESS;
}
