#include <cr/cr.h>

#include <core/assert.h>
#include <game.h>

static void
game_update_
(
  _In_ game_t                                             *game
)
{
  crude_input const *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( game->platform_node, crude_input );
  if ( input && input->should_close_window )
  {
    ecs_quit( game->engine->world );
  }
}

CR_EXPORT int
cr_main
(
  struct cr_plugin                                        *ctx,
  enum cr_op                                               operation
)
{
  CRUDE_ASSERT( ctx );

  if ( operation == CR_CLOSE )
  {
    return 0;
  }

  game_t *game = CRUDE_CAST( game_t*, ctx->userdata );
  game_update_( game );
 
  return 0;
}