#include <game.h>

int
main
(
)
{
  crude_engine                                             engine;
  game_t                                                   game;

  /* Initialization */
  {
    char                                                   temporary_buffer[ 1024 ];
    crude_engine_creation                                  engine_creation;
    
    engine_creation = CRUDE_COMPOUNT_EMPTY( crude_engine_creation );
    engine_creation.resource_path = CRUDE_GAME_RESOURCES_PATH;
    engine_creation.shader_path = CRUDE_GAME_SHADERS_PATH;
    crude_engine_initialize( &engine, &engine_creation );
    game_initialize( &game, &engine );
  }

  while ( engine.running )
  {
    crude_engine_update( &engine );
    game_update( &game );
  }
  
  /* Deinitialization */
  {
    game_deinitialize( &game );
    crude_engine_deinitialize( &engine );
  }

  return 0;
}