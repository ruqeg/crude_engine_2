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
    crude_editor_instance_intialize( );
    crude_editor_initialize( crude_editor_instance( ), &engine );
  }

   while ( engine.running )
   {
     crude_engine_update( &engine );
     crude_editor_postupdate( crude_editor_instance( ) );
   }
   
  /* Deinitialization */
  {
    crude_editor_deinitialize( crude_editor_instance( ) );
    crude_editor_instance_deintialize( );
    crude_engine_deinitialize( &engine );
  }

  return 0;
}
