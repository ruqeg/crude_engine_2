#include <core/log.h>
#include <core/profiler.h>
#include <scene/scripts_components.h>
#include <scene/scene_components.h>
#include <platform/platform_components.h>
#include <physics/physics_components.h>
#include <physics/physics_system.h>
#include <enemy_components.h>

#include <enemy_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_enemy_update_system_ );


static void
crude_enemy_update_system_
(
  _In_ ecs_iter_t *it
)
{
  crude_enemy *enemies_per_entity = ecs_field( it, crude_enemy, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_enemy                                           *enemy;
    enemy = &enemies_per_entity[ i ];
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_enemy_system )
{
  ECS_MODULE( world, crude_enemy_system );
  
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_enemy_components );
  
  //CRUDE_ECS_SYSTEM_DEFINE( world, crude_enemy_update_system_, EcsOnUpdate, NULL, {
  //  { .id = ecs_id( crude_enemy ) }
  //} );
}