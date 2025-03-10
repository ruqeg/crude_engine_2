#include <engine.h>
#include <cr.h>
#include <assert.h>

CR_EXPORT int
cr_main
(
  struct cr_plugin *ctx,
  enum cr_op        operation
)
{
  assert( ctx );
  if ( operation == CR_STEP )
  {
    // !TODO
  }
  return 0;
}
