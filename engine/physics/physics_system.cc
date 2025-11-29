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
  CRUDE_PROFILER_ZONE_NAME( "crude_physics_update_system_" );
  crude_physics_system_context *ctx = CRUDE_CAST( crude_physics_system_context*, it->ctx );
  
  // !TODO don't even ask T_T at least something
  //for ( float32 step_delta_time = 0.f; step_delta_time < it->delta_time + 0.001f; step_delta_time += CRUDE_PHYSICS_STEP_DELTA_TIME )
  //{
  crude_physics_update( ctx->physics, CRUDE_MIN( it->delta_time, 0.016f ) );
  //}
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
  
  CRUDE_ECS_SYSTEM_DEFINE( world, crude_physics_update_system_, EcsOnUpdate, ctx, { } );
}