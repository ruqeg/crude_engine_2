#include <engine/scene/scene_ecs.h>

#include <engine/audio/audio_ecs.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/

ECS_COMPONENT_DECLARE( crude_audio_listener );

void
crude_audio_components_import
(
  _In_ crude_ecs                                          *world
)
{
  CRUDE_ECS_MODULE( world, crude_audio_components );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_audio_listener );
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_audio_system );

CRUDE_ECS_SYSTEM_DECLARE( crude_audio_listener_update_system_ );

static void
crude_audio_listener_update_system_ 
(
  ecs_iter_t *it
)
{
  crude_audio_system_context                              *ctx;
  crude_transform                                         *transform_per_entity;
  crude_audio_listener                                    *listener_per_entity;

  ctx = CRUDE_CAST( crude_audio_system_context*, it->ctx );
  transform_per_entity = ecs_field( it, crude_transform, 0 );
  listener_per_entity = ecs_field( it, crude_audio_listener, 1 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_audio_listener                                  *listener;
    crude_entity                                           listener_node;
    
    transform = &transform_per_entity[ i ];
    listener = &listener_per_entity[ i ];
    listener_node = crude_entity_from_iterator( it, i );

    listener->last_local_to_world_update_time += it->delta_time;
    if ( listener->last_local_to_world_update_time > 0.016f )
    {
      crude_audio_device_listener_set_local_to_world( ctx->device, crude_transform_node_to_world( it->world, listener_node, transform ) );
      listener->last_local_to_world_update_time = 0.f;
    }
  }
}

void
crude_audio_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_audio_system_context                         *ctx
)
{
  crude_audio_components_import( world );
  crude_scene_components_import( world, manager );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_audio_listener_update_system_, EcsOnUpdate, ctx, { 
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_audio_listener ) },
  } );
}