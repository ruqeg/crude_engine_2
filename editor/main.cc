#include <engine/core/file.h>

#include <editor/editor.h>

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

    crude_editor_instance_intialize( );

    crude_editor_initialize( crude_editor_instance( ), &engine, working_directory );
  }

   while ( engine.running )
   {
     crude_engine_update( &engine );
     crude_editor_update( crude_editor_instance( ) );
   }
   
  /* Deinitialization */
  {
    crude_editor_deinitialize( crude_editor_instance( ) );
    crude_editor_instance_deintialize( );
    crude_engine_deinitialize( &engine );
  }

  return 0;
}
