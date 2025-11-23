#include <flecs.h>
#include <stdarg.h>

#include <engine/core/log.h>
#include <engine/core/array.h>
#include <engine/core/hash_map.h>
#include <engine/core/assert.h>
#include <engine/scene/scene_components.h>
#include <engine/physics/physics_components.h>

#include <engine/physics/physics.h>

void
crude_physics_initialize
(
  _In_ crude_physics                                      *physics,
  _In_ crude_physics_creation const                       *creation
)
{
  physics->manager = creation->manager;
  physics->collision_manager = creation->collision_manager;
  physics->simulation_enabled = true;
}

void
crude_physics_deinitialize
(
  _In_ crude_physics                                      *physics
)
{
}

void
crude_physics_update
(
  _In_ crude_physics                                      *physics,
  _In_ float64                                             delta_time
)
{
  if ( !physics->simulation_enabled )
  {
    return;
  }

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( physics->manager->character_bodies ); ++i )
  {
    crude_transform                                       *transform;
    crude_physics_collision_shape                         *collision_shape;
    crude_physics_character_body                          *character_body;
    crude_physics_character_body_handle                    character_body_handle;
    XMMATRIX                                               node_to_parent;
    XMMATRIX                                               parent_to_world;
    XMMATRIX                                               node_to_world;
    XMVECTOR                                               translation;
    XMVECTOR                                               velocity;

    character_body_handle = physics->manager->character_bodies[ i ];
    character_body = crude_physics_resources_manager_access_character_body( physics->manager, character_body_handle );

    transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( character_body->node, crude_transform );
    collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( character_body->node, crude_physics_collision_shape );

    velocity = XMLoadFloat3( &character_body->velocity );
    
    parent_to_world = crude_transform_parent_to_world( character_body->node );
    node_to_parent = crude_transform_node_to_parent( transform );
    node_to_world = XMMatrixMultiply( node_to_parent, parent_to_world );

    translation = XMVectorAdd( node_to_world.r[ 3 ], velocity * CRUDE_MIN( delta_time, 1.f ) );

    character_body->on_floor = false;

    for ( uint32 s = 0; s < CRUDE_ARRAY_LENGTH( physics->manager->static_bodies ); ++s )
    {
      crude_physics_static_body                           *second_body;
      crude_physics_collision_shape                       *second_collision_shape;
      crude_transform                                     *second_transform;
      XMMATRIX                                             second_transform_mesh_to_world;
      XMVECTOR                                             second_translation, closest_point;
      bool                                                 intersected;

      second_body = crude_physics_resources_manager_access_static_body( physics->manager, physics->manager->static_bodies[ s ] );

      if ( !( character_body->mask & second_body->layer ) )
      {
        continue;
      }

      second_collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( second_body->node, crude_physics_collision_shape );
      second_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( second_body->node, crude_transform );
      second_transform_mesh_to_world = crude_transform_node_to_world( second_body->node, second_transform );
      second_translation = second_transform_mesh_to_world.r[ 3 ];

      if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
      {
        XMVECTOR                                           second_box_half_extent;

        second_box_half_extent = XMLoadFloat3( &second_collision_shape->box.half_extent );
        closest_point = crude_closest_point_to_obb( translation, second_translation, second_box_half_extent, second_transform_mesh_to_world );
        intersected = crude_intersection_sphere_obb( closest_point, translation, collision_shape->sphere.radius );
      }
      else if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
      {
        crude_octree *octree = crude_collisions_resources_manager_access_octree( physics->collision_manager, second_collision_shape->mesh.octree_handle );
        closest_point = crude_octree_closest_point( octree, translation );
        intersected = crude_intersection_sphere_triangle( closest_point, translation, collision_shape->sphere.radius );
      }
      else
      {
        CRUDE_ASSERT( false );
      }
      
      if ( intersected )
      {
        crude_physics_collision_callback_container_fun( character_body->callback_container );

        if ( character_body->mask & 1 )
        {
          XMVECTOR                                         closest_point_to_translation;
          float32                                          translation_to_closest_point_projected_length;

          closest_point_to_translation = XMVectorSubtract( translation, closest_point );

          translation = XMVectorAdd( closest_point, XMVectorScale( XMVector3Normalize( closest_point_to_translation ), collision_shape->sphere.radius ) );
            
          closest_point_to_translation = XMVectorSubtract( translation, closest_point );
          translation_to_closest_point_projected_length = -1.f * XMVectorGetX( XMVector3Dot( closest_point_to_translation, XMVectorSet( 0, -1, 0, 1 ) ) );
          if ( translation_to_closest_point_projected_length > collision_shape->sphere.radius * 0.75f && translation_to_closest_point_projected_length < collision_shape->sphere.radius + 0.00001f ) // !TODO it works, so why not ahahah ( i like this solution :D )
          {
            character_body->on_floor = true;
          }
        }
      }
    }

    XMStoreFloat3( &transform->translation, XMVectorSubtract( translation, parent_to_world.r[ 3 ] ) );
  }
}

void
crude_physics_enable_simulation
(
  _In_ crude_physics                                      *physics,
  _In_ bool                                                enable
)
{
  physics->simulation_enabled = enable;

  if ( enable )
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( physics->manager->character_bodies ); ++i )
    {
      crude_physics_character_body *dynamic_body = crude_physics_resources_manager_access_character_body( physics->manager, physics->manager->character_bodies[ i ] );
      XMStoreFloat3( &dynamic_body->velocity, XMVectorZero( ) );
      dynamic_body->on_floor = false;
    }
  }
}

bool
crude_physics_cast_ray
(
  _In_ crude_physics                                      *physics,
  _In_ XMVECTOR                                            ray_origin,
  _In_ XMVECTOR                                            ray_direction,
  _In_ uint32                                              mask,
  _Out_opt_ crude_physics_raycast_result                  *result
)
{
  float32 nearest_t = FLT_MAX;

  for ( uint32 s = 0; s < CRUDE_ARRAY_LENGTH( physics->manager->static_bodies ); ++s )
  {
    crude_physics_static_body                           *second_body;
    crude_physics_collision_shape                       *second_collision_shape;
    crude_transform                                     *second_transform;
    XMMATRIX                                             second_transform_mesh_to_world;
    XMVECTOR                                             second_translation;
    crude_raycast_result                                 current_result;
    bool                                                 intersected;

    second_body = crude_physics_resources_manager_access_static_body( physics->manager, physics->manager->static_bodies[ s ] );

    if ( !( mask & second_body->layer ) )
    {
      continue;
    }

    second_collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( second_body->node, crude_physics_collision_shape );
    second_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( second_body->node, crude_transform );
    second_transform_mesh_to_world = crude_transform_node_to_world( second_body->node, second_transform );
    second_translation = second_transform_mesh_to_world.r[ 3 ];

    if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
    {
      XMVECTOR second_box_half_extent = XMLoadFloat3( &second_collision_shape->box.half_extent );
      intersected = crude_raycast_obb( ray_origin, ray_direction, second_translation, second_box_half_extent, second_transform_mesh_to_world, &current_result );
    }
    else if ( second_collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
    {
      crude_octree *octree = crude_collisions_resources_manager_access_octree( physics->collision_manager, second_collision_shape->mesh.octree_handle );
      intersected = crude_octree_cast_ray( octree, ray_origin, ray_direction, &current_result );
    }
    else
    {
      CRUDE_ASSERT( false );
    }
      
    if ( intersected && current_result.t < nearest_t )
    {
      nearest_t = current_result.t;
      if ( result )
      {
        result->raycast_result = current_result;
        result->node = second_body->node;
      }
    }
  }

  return nearest_t != FLT_MAX;
}