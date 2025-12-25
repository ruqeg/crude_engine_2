#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scripts_components.h>
#include <engine/scene/scene_components.h>
#include <engine/platform/platform.h>

#include <engine/scene/free_camera_system.h>

CRUDE_ECS_SYSTEM_DECLARE( crude_free_camera_update_system );

static void
crude_free_camera_update_system
(
  _In_ ecs_iter_t *it
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_free_camera_update_system" );
  crude_free_camera_system_context *ctx = CRUDE_CAST( crude_free_camera_system_context*, it->ctx );
  crude_transform *transforms = ecs_field( it, crude_transform, 0 );
  crude_free_camera *free_cameras = ecs_field( it, crude_free_camera, 1 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    if ( !free_cameras[ i ].input_enabled )
    {
      continue;
    }

    crude_input const *input = &ctx->platform->input;

    int32 moving_forward = input->keys[ SDL_SCANCODE_W ].current - input->keys[ SDL_SCANCODE_S ].current;
    int32 moving_up = input->keys[ SDL_SCANCODE_E ].current - input->keys[ SDL_SCANCODE_Q ].current;
    int32 moving_right = input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current;

    XMVECTOR translation = XMLoadFloat3( &transforms[ i ].translation );
    XMMATRIX node_to_world = crude_transform_node_to_world( crude_entity_from_iterator( it, ctx->world, i ), &transforms[ i ] );

    XMVECTOR basis_right = XMVector3Normalize( node_to_world.r[ 0 ] );
    XMVECTOR basis_up = XMVector3Normalize( node_to_world.r[ 1 ] );
    XMVECTOR basis_forward = XMVector3Normalize( node_to_world.r[ 2 ] );
    
    float32 moving_speed = free_cameras[ i ].moving_speed_multiplier;
    if ( input->keys[ SDL_SCANCODE_LSHIFT ].current )
    {
      moving_speed = moving_speed * 2.f;
    }

    if ( moving_right )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_right, moving_speed * moving_right * it->delta_time ) );
    }
    if ( moving_forward )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_forward, moving_speed * moving_forward * it->delta_time ) );
    }
    if ( moving_up )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_up, moving_speed * moving_up * it->delta_time ) );
    }
    XMStoreFloat3( &transforms[ i ].translation, translation );

    if ( input->mouse.right.current )
    {
      XMVECTOR rotation = XMLoadFloat4( &transforms[ i ].rotation );
      XMVECTOR camera_up = XMVectorGetY( basis_up ) > 0.0f ? g_XMIdentityR1 : XMVectorNegate( g_XMIdentityR1 );

      rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( basis_right, -free_cameras[ i ].rotating_speed_multiplier * input->mouse.rel.y ) );
      rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationAxis( camera_up, -free_cameras[ i ].rotating_speed_multiplier * input->mouse.rel.x ) );
      XMStoreFloat4( &transforms[ i ].rotation, rotation );
    }
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_free_camera_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_free_camera_system_context                   *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_free_camera_system );
  CRUDE_ECS_IMPORT( world, crude_scripts_components );
  CRUDE_ECS_IMPORT( world, crude_scene_components );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_free_camera_update_system, EcsOnUpdate, ctx, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_free_camera ) },
  } );
}