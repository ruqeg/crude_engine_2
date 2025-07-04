#include <flecs.h>

#include "launcher.h"

int
main
(
)
{
  crude_launcher                                           launcher;

  crude_launcher_initialize( &launcher );
  while ( launcher.engine.running )
  {
    crude_launcher_update( &launcher );
  }
  crude_launcher_deinitialize( &launcher );
  return 0;
}