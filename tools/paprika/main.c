#include <cr/cr.h>

#include <core/assert.h>
#include <paprika.h>

crude_paprika paprika_;

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

  crude_paprika *paprika = ctx->userdata;
  crude_paprika_update( paprika );
 
  return 0;
}