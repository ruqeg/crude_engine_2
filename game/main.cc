#include <engine/core/file.h>
#include <engine/core/profiler.h>

#include <game/game.h>

int
main
(
  _In_ int                                                argc,
  _In_ char                                              *argv[]
)
{
  crude_engine                                             engine;

  /* Initialization */
  {
    char                                                   working_directory[ 2048 ];
    crude_engine_creation                                  engine_creation;
    crude_game_creation                                    game_creation;

    engine_creation = CRUDE_COMPOUNT_EMPTY( crude_engine_creation );
    crude_engine_initialize( &engine, &engine_creation );

    game_instance_intialize( );
  
    game_creation = CRUDE_COMPOUNT_EMPTY( crude_game_creation );
    game_creation.engine = &engine;
    game_creation.framerate = 60;
    game_creation.resources_relative_directory = "\\..\\..\\resources\\";
    game_creation.render_graph_relative_directory = "\\..\\..\\resources\\";
    game_creation.scene_relative_filepath = "\\..\\..\\resources\\game\\nodes\\level0.crude_node";
    game_creation.shaders_relative_directory = "\\..\\..\\shaders\\";
    game_creation.techniques_relative_directory = "\\..\\..\\techniques\\";
    game_creation.compiled_shaders_relative_directory = "\\..\\..\\compiled_shaders\\";
    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    game_creation.working_absolute_directory = working_directory;
    game_initialize( game_instance( ), &game_creation );
  }

  while ( engine.running )
  {
    crude_engine_update( &engine );
    game_postupdate( game_instance( ) );
    CRUDE_PROFILER_MARK_FRAME;
  }
   
  /* Deinitialization */
  {
    game_deinitialize( game_instance( ) );
    game_instance_deintialize( );
    crude_engine_deinitialize( &engine );
  }

  return 0;
}