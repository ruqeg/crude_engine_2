#include <cr/cr.h>

#include <core/assert.h>
#include <dragoninn.h>

crude_dragoninn instance_;

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

  crude_dragoninn *instance_ = CRUDE_REINTERPRET_CAST( crude_dragoninn*, ctx->userdata );
  crude_dragoninn_update( instance_ );
 
  return 0;
}