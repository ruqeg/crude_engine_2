#include <engine/core/file.h>

#include <game/game.h>

int
main
(
)
{
  crude_engine                                             engine;

  /* Initialization */
  {
    char                                                   working_directory[ 4096 ];
    
    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    crude_engine_initialize( &engine, working_directory );

    crude_game_instance_intialize( );

    crude_game_initialize( crude_game_instance( ), &engine, working_directory );
  }

   while ( engine.running )
   {
     crude_engine_update( &engine );
     crude_game_update( crude_game_instance( ) );
   }
   
  /* Deinitialization */
  {
    crude_game_deinitialize( crude_game_instance( ) );
    crude_engine_deinitialize( &engine );
    crude_game_instance_deintialize( );
  }

  return 0;
}
