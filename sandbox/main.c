#include <cr/cr.h>

#include <core/assert.h>
#include <sandbox.h>

crude_sandbox sandbox_;

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

  crude_sandbox *sandbox = ctx->userdata;
  crude_sandbox_update( sandbox );
 
  return 0;
}