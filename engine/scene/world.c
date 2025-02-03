#include <scene/world.h>

crude_world* crude_world_create()
{
  return ecs_init();
}

void crude_world_destroy(crude_world* world)
{
  ecs_quit( world );
}
