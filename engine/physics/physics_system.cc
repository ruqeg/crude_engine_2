#include <physics/physics_components.h>
#include <physics/physics.h>

#include <physics/physics_system.h>

CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_destrotion_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_dynamic_body_destrotion_observer_ );

static void
crude_physics_static_body_destrotion_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_static_body_handle *static_bodies_per_entity = ecs_field( it, crude_physics_static_body_handle, 0 );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_destroy_static_body( crude_physics_instance( ), static_bodies_per_entity[ i ] );
  }
}

static void
crude_physics_dynamic_body_destrotion_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_dynamic_body_handle *dynamic_bodies_per_entity = ecs_field( it, crude_physics_dynamic_body_handle, 0 );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_destroy_dynamic_body( crude_physics_instance( ), dynamic_bodies_per_entity[ i ] );
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_system )
{
  ECS_MODULE( world, crude_physics_system );
  ECS_IMPORT( world, crude_physics_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_dynamic_body_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_physics_dynamic_body_handle ) },
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_physics_static_body_handle ) },
  } );
}