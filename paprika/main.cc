#include <cr/cr.h>

#include <core/assert.h>
#include <paprika.h>

crude_paprika paprika_;

static void
paprika_update_
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_input const *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( paprika->platform_node, crude_input );
  if ( input && input->should_close_window )
  {
    crude_paprika_deinitialize( paprika );
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

  crude_paprika *paprika = CRUDE_REINTERPRET_CAST( crude_paprika*, ctx->userdata );
  paprika_update_( paprika );
 
  return 0;
}