#if CRUDE_DEVELOP

#include <engine/core/assert.h>
#include <engine/physics/physics_components.h>
#include <engine/scene/scene_debug_components.h>

#include <engine/physics/physics_debug_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_physics_debug_system );

CRUDE_ECS_OBSERVER_DECLARE( crude_engine_physics_collision_shape_create_observer_ );

static void
crude_engine_physics_collision_shape_create_observer_ 
(
  ecs_iter_t *it
)
{
  crude_physics_debug_system_context *ctx = CRUDE_CAST( crude_physics_debug_system_context*, it->ctx );
  crude_physics_collision_shape *collision_shapes_per_entity = ecs_field( it, crude_physics_collision_shape, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    char const                                            *debug_model_relative_filename;
    char const                                            *debug_model_absolute_filename;
    crude_physics_collision_shape                         *collision_shape;
    crude_entity                                           node;

    collision_shape = &collision_shapes_per_entity[ i ];
    
    if ( collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
    {
      debug_model_relative_filename = "editor\\models\\crude_physics_box_collision_shape.gltf";
    }
    else if ( collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
    {
      debug_model_relative_filename = "editor\\models\\crude_physics_sphere_collision_shape.gltf";
    }
    else if ( collision_shape->type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
    {
      debug_model_relative_filename = collision_shape->mesh.model_filename;
    }
    else
    {
      CRUDE_ASSERT( false );
    }
  
    debug_model_absolute_filename = crude_string_buffer_append_use_f( ctx->string_bufffer, "%s%s", ctx->resources_absolute_directory, debug_model_relative_filename );

    node = CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } );
    CRUDE_ENTITY_SET_COMPONENT( node, crude_debug_collision, { debug_model_absolute_filename, true } );
  }
}


void
crude_physics_debug_system_import
(
  _In_ ecs_world_t                                        *world,
  _In_ crude_physics_debug_system_context                  *ctx
)
{
  ECS_IMPORT( world, crude_physics_components );
  ECS_IMPORT( world, crude_scene_debug_components );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_engine_physics_collision_shape_create_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_physics_collision_shape ) },
    { .id = ecs_id( crude_debug_collision ), .oper = EcsNot }
  } );
}

#endif