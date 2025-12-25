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
  ecs_query_desc_t                                         query_desc;
  
  physics->world = creation->world;
  physics->manager = creation->manager;
  physics->collision_manager = creation->collision_manager;
  physics->simulation_enabled = false;
  
  query_desc = CRUDE_COMPOUNT_EMPTY( ecs_query_desc_t );
  query_desc.terms[ 0 ].id = ecs_id( crude_physics_static_body_handle );
  query_desc.terms[ 1 ].id = ecs_id( crude_physics_collision_shape );
  query_desc.terms[ 2 ].id = ecs_id( crude_transform );
  physics->static_body_handle_query = crude_ecs_query_create( creation->world, &query_desc );
}

void
crude_physics_deinitialize
(
  _In_ crude_physics                                      *physics
)
{
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
  ecs_iter_t                                               static_body_handle_it;
  float32                                                  nearest_t;

  nearest_t = FLT_MAX;

  static_body_handle_it = ecs_query_iter( physics->world->handle, physics->static_body_handle_query );
  while ( ecs_query_next( &static_body_handle_it ) )
  {
    crude_physics_collision_shape                         *second_collision_shape_per_entity;
    crude_transform                                       *second_transform_per_entity;
    crude_physics_static_body_handle                      *second_body_handle_per_entity;

    second_body_handle_per_entity = ecs_field( &static_body_handle_it, crude_physics_static_body_handle, 0 );
    second_collision_shape_per_entity = ecs_field( &static_body_handle_it, crude_physics_collision_shape, 1 );
    second_transform_per_entity = ecs_field( &static_body_handle_it, crude_transform, 2 );

    for ( uint32 static_body_index = 0; static_body_index < static_body_handle_it.count; ++static_body_index )
    {
      crude_physics_collision_shape                       *second_collision_shape;
      crude_transform                                     *second_transform;
      crude_physics_static_body_handle                    *second_body_handle;
      crude_physics_static_body                           *second_body;
      crude_entity                                         static_body_node;
      XMMATRIX                                             second_transform_mesh_to_world;
      XMVECTOR                                             second_translation;
      crude_raycast_result                                 current_result;
      bool                                                 intersected;
      
      second_collision_shape = &second_collision_shape_per_entity[ static_body_index ];
      second_transform = &second_transform_per_entity[ static_body_index ];
      second_body_handle = &second_body_handle_per_entity[ static_body_index ];
      static_body_node = crude_entity_from_iterator( &static_body_handle_it, physics->world, static_body_index );
    
      second_body = crude_physics_resources_manager_access_static_body( physics->manager, *second_body_handle );
  
      if ( !second_body->enabeld )
      {
        continue;
      }

      if ( !( mask & second_body->layer ) )
      {
        continue;
      }
  
      second_collision_shape = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( static_body_node, crude_physics_collision_shape );
      second_transform = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( static_body_node, crude_transform );
      second_transform_mesh_to_world = crude_transform_node_to_world( static_body_node, second_transform );
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
          result->node = static_body_node;
          result->body_layer = second_body->layer;
        }
      }
    }
  }

  return nearest_t != FLT_MAX;
}