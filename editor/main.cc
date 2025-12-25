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
    crude_editor_creation                                  editor_creation;
    
    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    crude_engine_initialize( &engine, working_directory );

    crude_editor_instance_intialize( );

    editor_creation = CRUDE_COMPOUNT_EMPTY( crude_editor_creation );
    editor_creation.engine = &engine;
    editor_creation.framerate = 60;
    editor_creation.resources_relative_directory = "\\..\\..\\resources\\";
    editor_creation.render_graph_relative_directory = "\\..\\..\\resources\\";
    editor_creation.scene_relative_filepath = "\\..\\..\\resources\\game\\nodes\\level0.crude_node";
    editor_creation.shaders_relative_directory = "\\..\\..\\shaders\\";
    editor_creation.techniques_relative_directory = "\\..\\..\\techniques\\";
    editor_creation.compiled_shaders_relative_directory = "\\..\\..\\compiled_shaders\\";
    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    editor_creation.working_absolute_directory = working_directory;
    crude_editor_initialize( crude_editor_instance( ), &editor_creation );
  }

   while ( engine.running )
   {
     crude_engine_update( &engine );
   }
   
  /* Deinitialization */
  {
    crude_editor_deinitialize( crude_editor_instance( ) );
    crude_editor_instance_deintialize( );
    crude_engine_deinitialize( &engine );
  }

  return 0;
}
