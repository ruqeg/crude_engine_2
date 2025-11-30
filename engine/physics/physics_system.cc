#include <engine/core/assert.h>
#include <engine/core/profiler.h>
#include <engine/physics/physics_components.h>
#include <engine/scene/scene_components.h>

#include <engine/physics/physics_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_physics_system );

CRUDE_ECS_OBSERVER_DECLARE( crude_physics_static_body_handle_destroy_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_physics_character_body_handle_destroy_observer_ );
CRUDE_ECS_SYSTEM_DECLARE( crude_physics_update_system_ );

static void
crude_physics_static_body_handle_destroy_observer_ 
(
  ecs_iter_t *it
)
{
  crude_physics_system_context *ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  crude_physics_static_body_handle *static_body_handle_per_entity = ecs_field( it, crude_physics_static_body_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_resources_manager_destroy_static_body( ctx->physics->manager, static_body_handle_per_entity[ i ] );
  }
}

static void
crude_physics_character_body_handle_destroy_observer_ 
(
  ecs_iter_t *it
)
{
  crude_physics_system_context *ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  crude_physics_character_body_handle *character_body_handle_per_entity = ecs_field( it, crude_physics_character_body_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_physics_resources_manager_destroy_character_body( ctx->physics->manager, character_body_handle_per_entity[ i ] );
  }
}

static void
crude_physics_update_system_ 
(
  ecs_iter_t *it
)
{
  crude_physics_character_body_handle                     *character_body_handle_per_entity;
  crude_physics_collision_shape                           *collision_shape_per_entity;
  crude_transform                                         *transform_per_entity;
  crude_physics_system_context                            *ctx;
  float32                                                  delta_time;
  
  CRUDE_PROFILER_ZONE_NAME( "crude_physics_update_system_" );

  ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  character_body_handle_per_entity = ecs_field( it, crude_physics_character_body_handle, 0 );
  collision_shape_per_entity = ecs_field( it, crude_physics_collision_shape, 1 );
  transform_per_entity = ecs_field( it, crude_transform, 2 );
  
  if ( !ctx->physics->simulation_enabled )
  {
    return;
  }

  delta_time = CRUDE_MIN( it->delta_time, 0.016f );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_physics_collision_shape                         *collision_shape;
    crude_physics_character_body                          *character_body;
    crude_entity                                           character_body_node;
    ecs_iter_t                                             static_body_handle_it;
    crude_physics_character_body_handle                    character_body_handle;
    XMMATRIX                                               node_to_parent;
    XMMATRIX                                               parent_to_world;
    XMMATRIX                                               node_to_world;
    XMVECTOR                                               translation;
    XMVECTOR                                               velocity;
    
    character_body_handle = character_body_handle_per_entity[ i ];
    collision_shape = &collision_shape_per_entity[ i ];
    transform = &transform_per_entity[ i ];

    character_body_node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );

    character_body = crude_physics_resources_manager_access_character_body( ctx->physics->manager, character_body_handle );

    velocity = XMLoadFloat3( &character_body->velocity );
    
    parent_to_world = crude_transform_parent_to_world( character_body_node );
    node_to_parent = crude_transform_node_to_parent( transform );
    node_to_world = XMMatrixMultiply( node_to_parent, parent_to_world );
    
    translation = XMVectorAdd( node_to_world.r[ 3 ], velocity * CRUDE_MIN( delta_time, 1.f ) );

    character_body->on_floor = false;

    static_body_handle_it = ecs_query_iter( it->world, ctx->physics->static_body_handle_query );
    while ( ecs_query_next( &static_body_handle_it ) )
    {
      crude_physics_collision_shape                       *second_collision_shape_per_entity;
      crude_transform                                     *second_transform_per_entity;
      crude_physics_static_body_handle                    *second_body_handle_per_entity;

      second_body_handle_per_entity = ecs_field( &static_body_handle_it, crude_physics_static_body_handle, 0 );
      second_collision_shape_per_entity = ecs_field( &static_body_handle_it, crude_physics_collision_shape, 1 );
      second_transform_per_entity = ecs_field( &static_body_handle_it, crude_transform, 2 );

      for ( uint32 static_body_index = 0; static_body_index < static_body_handle_it.count; ++static_body_index )
      {
        crude_physics_collision_shape                     *second_collision_shape;
        crude_transform                                   *second_transform;
        crude_physics_static_body_handle                  *second_body_handle;
        crude_physics_static_body                         *second_body;
        crude_entity                                       second_body_node;
        XMMATRIX                                           second_transform_mesh_to_world;
        XMVECTOR                                           second_translation, closest_point;
        bool                                               intersected;
        
        second_collision_shape = &second_collision_shape_per_entity[ static_body_index ];
        second_transform = &second_transform_per_entity[ static_body_index ];
        second_body_handle = &second_body_handle_per_entity[ static_body_index ];
        second_body_node = CRUDE_COMPOUNT( crude_entity, { static_body_handle_it.entities[ static_body_index ], static_body_handle_it.world } );
     
        second_body = crude_physics_resources_manager_access_static_body( ctx->physics->manager, *second_body_handle );

        if ( !second_body->enabeld )
        {
          continue;
        }

        if ( !( character_body->mask & second_body->layer ) )
        {
          continue;
        }

        second_transform_mesh_to_world = crude_transform_node_to_world( second_body_node, second_transform );
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
          crude_octree *octree = crude_collisions_resources_manager_access_octree( ctx->physics->collision_manager, second_collision_shape->mesh.octree_handle );
          closest_point = crude_octree_closest_point( octree, translation );
          intersected = crude_intersection_sphere_triangle( closest_point, translation, collision_shape->sphere.radius );
        }
        else
        {
          CRUDE_ASSERT( false );
        }
        
        if ( intersected )
        {
          crude_physics_collision_callback_container_fun( character_body->callback_container, character_body_node, second_body_node );

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
    }

    XMStoreFloat3( &transform->translation, XMVectorSubtract( translation, parent_to_world.r[ 3 ] ) );
  }

  CRUDE_PROFILER_END;
}

void
crude_physics_system_import
(
  _In_ ecs_world_t                                        *world,
  _In_ crude_physics_system_context                       *ctx
)
{
  ECS_IMPORT( world, crude_physics_components );

  //CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_static_body_handle_destroy_observer_, EcsOnRemove, ctx, { 
  //  { .id = ecs_id( crude_physics_static_body_handle ) }
  //} );
  //
  //CRUDE_ECS_OBSERVER_DEFINE( world, crude_physics_character_body_handle_destroy_observer_, EcsOnRemove, ctx, { 
  //  { .id = ecs_id( crude_physics_character_body_handle ) }
  //} );
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_update_system_, EcsOnUpdate, ctx, { 
    { .id = ecs_id( crude_physics_character_body_handle ) },
    { .id = ecs_id( crude_physics_collision_shape ) },
    { .id = ecs_id( crude_transform ) }
  } );
}