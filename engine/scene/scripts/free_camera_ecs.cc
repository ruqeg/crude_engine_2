#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/platform/platform.h>

#include <engine/scene/scripts/free_camera_ecs.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_free_camera );
CRUDE_COMPONENT_STRING_DEFINE( crude_free_camera, "crude_free_camera" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_free_camera )
{
  crude_memory_set( component, 0, sizeof( crude_free_camera ) );
  component->moving_speed_multiplier = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "moving_speed_multiplier" ) );
  component->rotating_speed_multiplier = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "rotating_speed_multiplier" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_free_camera )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( "crude_free_camera" ) );
  cJSON_AddItemToObject( free_camera_json, "moving_speed_multiplier", cJSON_CreateNumber( component->moving_speed_multiplier ) );
  cJSON_AddItemToObject( free_camera_json, "rotating_speed_multiplier", cJSON_CreateNumber( component->rotating_speed_multiplier ) );
  return free_camera_json;
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_free_camera_update_system_ );

void
crude_free_camera_update_system_
(
  _In_ ecs_iter_t                                         *it
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

    crude_input const *input = ctx->input;

    int32 moving_forward = input->keys[ SDL_SCANCODE_W ].current - input->keys[ SDL_SCANCODE_S ].current;
    int32 moving_up = input->keys[ SDL_SCANCODE_E ].current - input->keys[ SDL_SCANCODE_Q ].current;
    int32 moving_right = input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current;

    XMVECTOR translation = XMLoadFloat3( &transforms[ i ].translation );
    XMMATRIX node_to_world = crude_transform_node_to_world( it->world, crude_entity_from_iterator( it, i ), &transforms[ i ] );

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
      translation = XMVectorAdd( translation, XMVectorScale( basis_right, moving_speed * moving_right * it->delta_time * 1.f ) );
    }
    if ( moving_forward )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_forward, moving_speed * moving_forward * it->delta_time * -1.f ) );
    }
    if ( moving_up )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_up, moving_speed * moving_up * it->delta_time * 1.f ) );
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
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_free_camera_system_context                   *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_free_camera_system );
  
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_free_camera );

  crude_scene_components_import( world, manager );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_free_camera_update_system_, EcsOnUpdate, ctx, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_free_camera ) },
  } );
}