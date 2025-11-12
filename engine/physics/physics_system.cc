#include <core/array.h>
#include <scene/scene_components.h>
#include <physics/physics_components.h>
#include <physics/physics_intersections.h>
#include <physics/physics.h>

#include <physics/physics_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_physics_dynamic_body_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_destrotion_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_dynamic_body_destrotion_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_dynamic_body_transform_set_observer_ );

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

static void
crude_physics_dynamic_body_update_system_
(
  ecs_iter_t *it
)
{
  if ( !crude_physics_instance( )->simulation_enabled )
  {
    return;
  }
  crude_physics_dynamic_body_handle *dynamic_bodies_per_entity = ecs_field( it, crude_physics_dynamic_body_handle, 0 );
  crude_physics_collision_shape *collision_shapes_per_entity = ecs_field( it, crude_physics_collision_shape, 1 );
  crude_transform *transforms_per_entity = ecs_field( it, crude_transform, 2 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_physics_collision_shape                         *collision_shape;
    crude_physics_dynamic_body_handle                      dynamic_body_handle;
    XMVECTOR                                               translation;
    XMVECTOR                                               velocity;

    dynamic_body_handle = dynamic_bodies_per_entity[ i ];
    transform = &transforms_per_entity[ i ];
    collision_shape = &collision_shapes_per_entity[ i ];

    velocity = crude_physics_dynamic_body_get_velocity( crude_physics_instance( ), dynamic_body_handle );

    translation = XMLoadFloat3( &transform->translation );
    
    translation = XMVectorAdd( translation, velocity * it->delta_time );

    for ( uint32 s = 0; s < CRUDE_ARRAY_LENGTH( crude_physics_instance( )->static_bodies ); ++s )
    {
      crude_physics_static_body *second_body = crude_physics_access_static_body( crude_physics_instance( ), crude_physics_instance( )->static_bodies[ i ] );
      crude_physics_collision_shape *second_collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( second_body->node, crude_physics_collision_shape );
      crude_transform *second_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( second_body->node, crude_transform );
      XMMATRIX second_transform_mesh_to_world = crude_transform_node_to_world( second_body->node, second_transform );

      if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
      {
        XMVECTOR closest_point = crude_physics_closest_point_to_obb(
          translation,
          XMLoadFloat3( &second_transform->translation ),
          XMLoadFloat3( &second_collision_shape->box.half_extent ),
          second_transform_mesh_to_world );
        
        if ( crude_physics_intersection_sphere_obb( closest_point, translation, collision_shape->sphere.radius  ) )
        {
          crude_physics_dynamic_body_set_velocity( crude_physics_instance( ), dynamic_body_handle, XMVectorZero( ) );
          translation = XMVectorAdd( closest_point, XMVectorScale( XMVector3Normalize( XMVectorSubtract( translation, closest_point ) ), collision_shape->sphere.radius ) );
        }
      }
    }

    XMStoreFloat3( &transform->translation, translation );
  }
}

static void
crude_physics_dynamic_body_transform_set_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_dynamic_body_handle *dynamic_bodies_per_entity = ecs_field( it, crude_physics_dynamic_body_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_dynamic_body_handle dynamic_body_handle = dynamic_bodies_per_entity[ i ];
    crude_physics_dynamic_body_set_velocity( crude_physics_instance( ), dynamic_body_handle, XMVectorZero( ) );
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_system )
{
  ECS_MODULE( world, crude_physics_system );
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_dynamic_body_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_physics_dynamic_body_handle ) },
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_physics_static_body_handle ) },
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_dynamic_body_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_physics_dynamic_body_handle ) },
    { .id = ecs_id( crude_physics_collision_shape ) },
    { .id = ecs_id( crude_transform ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_dynamic_body_transform_set_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_physics_dynamic_body_handle ) },
    { .id = ecs_id( crude_transform ) }
  } );
}