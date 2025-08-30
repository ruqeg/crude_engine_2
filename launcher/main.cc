#include <flecs.h>

#define CR_HOST
#include <cr/cr.h>

#include <core/string.h>
#include <core/process.h>
#include <core/file.h>
#include <core/time.h>
#include <game.h>

static char*
get_plugin_dll_path_
(
  _In_ char                                               *buffer,
  _In_ uint32                                              buffer_size,
  _In_ char const                                         *plugin_dll
);

int
main
(
)
{
  cr_plugin                                                engine_simulation_cr;
  cr_plugin                                                game_cr;
  crude_engine                                             engine;
  game_t                                                   game;

  /* Initialization */
  {
    char                                                   temporary_buffer[ 1024 ];
    crude_engine_creation                                  engine_creation;
    
    engine_creation = CRUDE_COMPOUNT_EMPTY( crude_engine_creation );
    crude_engine_initialize( &engine, &engine_creation );
    game_initialize( &game, &engine );
    
    cr_plugin_open( game_cr, get_plugin_dll_path_( temporary_buffer, sizeof( temporary_buffer ), CR_PLUGIN( "game" ) ) );
    cr_plugin_open( engine_simulation_cr, get_plugin_dll_path_( temporary_buffer, sizeof( temporary_buffer ), CR_PLUGIN( "crude_engine_simulation" ) ) );
    
    game_cr.userdata = &game;
    engine_simulation_cr.userdata = &engine;
  }

  while ( engine.running )
  {
    cr_plugin_update( game_cr );
    cr_plugin_update( engine_simulation_cr );
    cr_plat_init( );
  }
  
  /* Deinitialization */
  {
    cr_plugin_close( game_cr );
    cr_plugin_close( engine_simulation_cr );
  
    game_deinitialize( &game );
    crude_engine_deinitialize( &engine );
  }

  return 0;
}

char*
get_plugin_dll_path_
(
  _In_ char                                               *buffer,
  _In_ uint32                                              buffer_size,
  _In_ char const                                         *plugin_dll
)
{
  buffer[ 0 ] = 0u;
  crude_get_executable_directory( buffer, buffer_size );
  crude_string_cat( buffer, plugin_dll, buffer_size );
  return buffer;
}