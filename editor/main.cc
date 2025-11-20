#include <editor.h>

int
main
(
)
{
  crude_engine                                             engine;

  /* Initialization */
  {
    char                                                   temporary_buffer[ 1024 ];
    crude_engine_creation                                  engine_creation;
    
    engine_creation = CRUDE_COMPOUNT_EMPTY( crude_engine_creation );
    crude_engine_initialize( &engine, &engine_creation );
    game_instance_intialize( );
    game_initialize( game_instance( ), &engine );
  }

   while ( engine.running )
   {
     crude_engine_update( &engine );
     game_postupdate( game_instance( ) );
   }
   
  /* Deinitialization */
  {
    game_deinitialize( game_instance( ) );
    game_instance_deintialize( );
    crude_engine_deinitialize( &engine );
  }

  return 0;
}
