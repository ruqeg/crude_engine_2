#include <flecs.h>

#include <core/time.h>

#include "launcher.h"

int
main
(
)
{
  crude_launcher                                           launcher;
  int64                                                    last_update_time;

  
  crude_launcher_initialize( &launcher );
  
  last_update_time = crude_time_now();
  while ( launcher.engine.running )
  {
    int64 current_time = crude_time_now();
    float32 delta = crude_time_delta_seconds( last_update_time, current_time );
    if ( crude_time_delta_seconds( last_update_time, current_time ) >= 0.016f )
    {
      crude_launcher_update( &launcher );
      last_update_time = current_time;
    }
  }
  crude_launcher_deinitialize( &launcher );
  return 0;
}