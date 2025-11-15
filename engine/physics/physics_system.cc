#include <core/assert.h>
#include <core/log.h>
#include <core/array.h>
#include <scene/scene_components.h>
#include <physics/physics_components.h>
#include <physics/physics.h>

#include <physics/physics_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_physics_character_body_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_collision_shape_creation_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_destrotion_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_character_body_destrotion_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_character_body_transform_set_observer_ );

static void
crude_physics_collision_shape_creation_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_collision_shape *collision_shapes_per_entity = ecs_field( it, crude_physics_collision_shape, 0 );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    if ( collision_shapes_per_entity[ i ].type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
    {
      collision_shapes_per_entity[ i ].debug_model_filename = "editor\\models\\crude_physics_box_collision_shape.gltf";
    }
    else if ( collision_shapes_per_entity[ i ].type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
    {
      collision_shapes_per_entity[ i ].debug_model_filename = "editor\\models\\crude_physics_sphere_collision_shape.gltf";
    }
    else
    {
      CRUDE_ASSERT( false );
    }
  }
}

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
crude_physics_character_body_destrotion_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_character_body_handle *dynamic_bodies_per_entity = ecs_field( it, crude_physics_character_body_handle, 0 );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_destroy_dynamic_body( crude_physics_instance( ), dynamic_bodies_per_entity[ i ] );
  }
}

static void
crude_physics_character_body_update_system_
(
  ecs_iter_t *it
)
{
  crude_physics_character_body_handle                       *dynamic_bodies_per_entity;
  crude_physics_collision_shape                           *collision_shapes_per_entity;
  crude_transform                                         *transforms_per_entity;

  if ( !crude_physics_instance( )->simulation_enabled )
  {
    return;
  }

  dynamic_bodies_per_entity = ecs_field( it, crude_physics_character_body_handle, 0 );
  collision_shapes_per_entity = ecs_field( it, crude_physics_collision_shape, 1 );
  transforms_per_entity = ecs_field( it, crude_transform, 2 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_physics_collision_shape                         *collision_shape;
    crude_physics_character_body                          *dynamic_body;
    crude_physics_character_body_handle                    dynamic_body_handle;
    XMVECTOR                                               translation;
    XMVECTOR                                               velocity;

    dynamic_body_handle = dynamic_bodies_per_entity[ i ];
    transform = &transforms_per_entity[ i ];
    collision_shape = &collision_shapes_per_entity[ i ];

    dynamic_body = crude_physics_access_dynamic_body( crude_physics_instance( ), dynamic_body_handle );

    velocity = crude_physics_character_body_get_velocity( crude_physics_instance( ), dynamic_body_handle );

    translation = XMLoadFloat3( &transform->translation );
    
    translation = XMVectorAdd( translation, velocity * CRUDE_MIN( it->delta_time, 1.f ) );

    dynamic_body->on_floor = false;

    for ( uint32 s = 0; s < CRUDE_ARRAY_LENGTH( crude_physics_instance( )->static_bodies ); ++s )
    {
      crude_physics_static_body                           *second_body;
      crude_physics_collision_shape                       *second_collision_shape;
      crude_transform                                     *second_transform;
      XMMATRIX                                             second_transform_mesh_to_world;
      XMVECTOR                                             second_translation;

      second_body = crude_physics_access_static_body( crude_physics_instance( ), crude_physics_instance( )->static_bodies[ s ] );
      second_collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( second_body->node, crude_physics_collision_shape );
      second_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( second_body->node, crude_transform );
      second_transform_mesh_to_world = crude_transform_node_to_world( second_body->node, second_transform );
      second_translation = XMLoadFloat3( &second_transform->translation );

      if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
      {
        XMVECTOR                                           second_box_half_extent, closest_point, closest_point_to_translation;
        float32                                            translation_to_closest_point_projected_length;

        second_box_half_extent = XMLoadFloat3( &second_collision_shape->box.half_extent );
        closest_point = crude_closest_point_to_obb( translation, second_translation, second_box_half_extent, second_transform_mesh_to_world );
        
        if ( crude_intersection_sphere_obb( closest_point, translation, collision_shape->sphere.radius  ) )
        {
          closest_point_to_translation = XMVectorSubtract( translation, closest_point );
          translation = XMVectorAdd( closest_point, XMVectorScale( XMVector3Normalize( closest_point_to_translation ), collision_shape->sphere.radius ) );
          
          closest_point_to_translation = XMVectorSubtract( translation, closest_point );
          translation_to_closest_point_projected_length = -1.f * XMVectorGetX( XMVector3Dot( closest_point_to_translation, XMVectorSet( 0, -1, 0, 1 ) ) );
          if ( translation_to_closest_point_projected_length > collision_shape->sphere.radius * 0.75f && translation_to_closest_point_projected_length < collision_shape->sphere.radius + 0.00001f ) // !TODO it works, so why not ahahah ( i like this solution :D )
          {
            dynamic_body->on_floor = true;
          }
        }
      }
    }

    XMStoreFloat3( &transform->translation, translation );
  }
}

static void
crude_physics_character_body_transform_set_observer_
(
  ecs_iter_t *it
)
{
  crude_physics_character_body_handle *dynamic_bodies_per_entity = ecs_field( it, crude_physics_character_body_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_character_body_handle dynamic_body_handle = dynamic_bodies_per_entity[ i ];
    crude_physics_character_body_set_velocity( crude_physics_instance( ), dynamic_body_handle, XMVectorZero( ) );
  }
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_physics_system )
{
  ECS_MODULE( world, crude_physics_system );
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_physics_components );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_collision_shape_creation_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_physics_collision_shape ) },
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_character_body_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_physics_character_body_handle ) },
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_destrotion_observer_, EcsOnRemove, { 
    { .id = ecs_id( crude_physics_static_body_handle ) },
  } );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_character_body_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_physics_character_body_handle ) },
    { .id = ecs_id( crude_physics_collision_shape ) },
    { .id = ecs_id( crude_transform ) }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_character_body_transform_set_observer_, EcsOnSet, { 
    { .id = ecs_id( crude_physics_character_body_handle ) },
    { .id = ecs_id( crude_transform ) }
  } );
}