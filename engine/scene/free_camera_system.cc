#include <core/log.h>
#include <core/profiler.h>
#include <scene/scripts_components.h>
#include <scene/scene_components.h>
#include <platform/platform_components.h>

#include <scene/free_camera_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_free_camera_update_system );

static void
crude_free_camera_update_system
(
  _In_ ecs_iter_t *it
)
{
  crude_transform *transforms = ecs_field( it, crude_transform, 0 );
  crude_free_camera *free_cameras = ecs_field( it, crude_free_camera, 1 );
  
  CRUDE_PROFILER_ZONE_NAME( "UpdateFreeCameras" );
  for ( uint32 i = 0; i < it->count; ++i )
  {
    if ( !free_cameras[ i ].enabled )
    {
      continue;
    }

    crude_input const *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( free_cameras[ i ].entity_input, crude_input );

    int32 moving_forward = input->keys[ 'w' ].current - input->keys[ 's' ].current;
    int32 moving_up = input->keys[ 'e' ].current - input->keys[ 'q' ].current;
    int32 moving_right = input->keys[ 'd' ].current - input->keys[ 'a' ].current;

    XMVECTOR translation = XMLoadFloat3( &transforms[ i ].translation );
    XMMATRIX node_to_world = crude_transform_node_to_world( CRUDE_COMPOUNT( crude_entity, { it->entities[ i ], it->world } ), &transforms[ i ] );

    XMVECTOR basis_right = XMVector3Normalize( node_to_world.r[ 0 ] );
    XMVECTOR basis_up = XMVector3Normalize( node_to_world.r[ 1 ] );
    XMVECTOR basis_forward = XMVector3Normalize( node_to_world.r[ 2 ] );

    if ( moving_right )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_right, free_cameras[i].moving_speed_multiplier.x * moving_right * it->delta_time ) );
    }
    if ( moving_forward )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_forward, free_cameras[i].moving_speed_multiplier.z * moving_forward * it->delta_time ) );
    }
    if ( moving_up )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_up, free_cameras[i].moving_speed_multiplier.y * moving_up * it->delta_time ) );
    }
    XMStoreFloat3( &transforms[ i ].translation, translation );

    if ( input->mouse.right.current )
    {
      XMVECTOR rotation = XMLoadFloat4( &transforms[ i ].rotation );
      XMVECTOR camera_up = XMVectorGetY( basis_up ) > 0.0f ? g_XMIdentityR1 : XMVectorNegate( g_XMIdentityR1 );

      rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( basis_right, -free_cameras[ i ].rotating_speed_multiplier.y * input->mouse.rel.y * it->delta_time ) );
      rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( camera_up, -free_cameras[ i ].rotating_speed_multiplier.x * input->mouse.rel.x * it->delta_time ) );
      XMStoreFloat4( &transforms[ i ].rotation, rotation );
    }
  }
  CRUDE_PROFILER_END;
}

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_free_camera_system )
{
  ECS_MODULE( world, crude_free_camera_system );
  ECS_IMPORT( world, crude_scripts_components );
  ECS_IMPORT( world, crude_scene_components );
  ECS_IMPORT( world, crude_platform_components );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_free_camera_update_system, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_free_camera ) },
  } );
}