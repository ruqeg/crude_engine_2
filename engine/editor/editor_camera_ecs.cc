#include <engine/core/log.h>
#include <engine/core/profiler.h>
#include <engine/scene/scene_ecs.h>
#include <engine/platform/platform.h>
#include <engine/editor/editor.h>
#include <engine/engine.h>

#include <engine/editor/editor_camera_ecs.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
ECS_COMPONENT_DECLARE( crude_editor_camera );
CRUDE_COMPONENT_STRING_DEFINE( crude_editor_camera, "crude_editor_camera" );

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_editor_camera )
{
  crude_memory_set( component, 0, sizeof( crude_editor_camera ) );
  component->walk_speed = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "walk_speed" ) );
  component->rotate_speed = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "rotate_speed" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_editor_camera )
{
  cJSON *free_camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( free_camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_editor_camera ) ) );
  cJSON_AddItemToObject( free_camera_json, "walk_speed", cJSON_CreateNumber( component->walk_speed ) );
  cJSON_AddItemToObject( free_camera_json, "rotate_speed", cJSON_CreateNumber( component->rotate_speed ) );
  return free_camera_json;
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_editor_camera_update_system_ );

void
crude_editor_camera_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_input const                                       *input;
  crude_platform                                          *platform;
  crude_editor_camera_system_context                      *ctx;
  crude_transform                                         *transforms_per_entity;
  crude_editor_camera                                     *editor_cameras_per_entity;

  CRUDE_PROFILER_ZONE_NAME( "crude_free_camera_update_system" );
  ctx = CRUDE_CAST( crude_editor_camera_system_context*, it->ctx );
  transforms_per_entity = ecs_field( it, crude_transform, 0 );
  editor_cameras_per_entity = ecs_field( it, crude_editor_camera, 1 );

  platform = ctx->platform;
  input = &platform->input;

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_transform                                       *transform;
    crude_editor_camera                                   *editor_camera;
    crude_entity                                           entity;
    XMFLOAT3                                               move_direction;
    XMVECTOR                                               translation;
    XMVECTOR                                               basis_right, basis_forward, basis_up;
    XMMATRIX                                               editor_camera_node_to_world;
    float32                                                move_speed;

    transform = &transforms_per_entity[ i ];
    editor_camera = &editor_cameras_per_entity[ i ];

    if ( !editor_camera->input_enabled )
    {
      continue;
    }

    entity = crude_entity_from_iterator( it, i );
    
    move_direction.x = input->keys[ SDL_SCANCODE_D ].current - input->keys[ SDL_SCANCODE_A ].current;
    move_direction.y = input->keys[ SDL_SCANCODE_E ].current - input->keys[ SDL_SCANCODE_Q ].current;
    move_direction.z = input->keys[ SDL_SCANCODE_W ].current - input->keys[ SDL_SCANCODE_S ].current;

    translation = XMLoadFloat3( &transform->translation );
    editor_camera_node_to_world = crude_transform_node_to_world( it->world, entity, transform );

    basis_right = XMVector3Normalize( editor_camera_node_to_world.r[ 0 ] );
    basis_up = XMVector3Normalize( editor_camera_node_to_world.r[ 1 ] );
    basis_forward = XMVector3Normalize( editor_camera_node_to_world.r[ 2 ] );
    
    move_speed = editor_camera->walk_speed;
    if ( input->keys[ SDL_SCANCODE_LSHIFT ].current )
    {
      move_speed *= 2.f;
    }

    if ( move_direction.x )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_right, move_direction.x * move_speed * it->delta_time * 1.f ) );
    }
    if ( move_direction.y )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_up, move_direction.y * move_speed * it->delta_time * 1.f ) );
    }
    if ( move_direction.z )
    {
      translation = XMVectorAdd( translation, XMVectorScale( basis_forward, move_direction.z * move_speed * it->delta_time * -1.f ) );
    }

    XMStoreFloat3( &transform->translation, translation );

    if ( input->mouse.right.current )
    {
      XMVECTOR                                             editor_rotation;
      XMVECTOR                                             editor_camera_up_axis;

      editor_camera_up_axis = XMVectorGetY( basis_up ) > 0.0f ? g_XMIdentityR1 : XMVectorNegate( g_XMIdentityR1 );

      editor_rotation = XMLoadFloat4( &transform->rotation );
      editor_rotation = XMQuaternionMultiply( editor_rotation, XMQuaternionRotationAxis( basis_right, -editor_camera->rotate_speed * input->mouse.rel.y ) );
      editor_rotation = XMQuaternionMultiply( editor_rotation, XMQuaternionRotationAxis( editor_camera_up_axis, -editor_camera->rotate_speed * input->mouse.rel.x ) );
      XMStoreFloat4( &transform->rotation, editor_rotation );
    }
    
    if ( input->mouse.right.current )
    {
      if ( !crude_platform_cursor_hidden( platform ) )
      {
        SDL_GetMouseState( &editor_camera->last_unrelative_mouse_position.x, &editor_camera->last_unrelative_mouse_position.y );
        crude_platform_hide_cursor( platform );
      }
    }
    else
    {
      if ( crude_platform_cursor_hidden( platform ) )
      {
        SDL_WarpMouseInWindow( platform->sdl_window, editor_camera->last_unrelative_mouse_position.x, editor_camera->last_unrelative_mouse_position.y );
        crude_platform_show_cursor( platform );
      }
    }
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_editor_camera_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_editor_camera_system_context                 *ctx
)
{
  CRUDE_ECS_MODULE( world, crude_editor_camera_system );
  
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_editor_camera );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_editor_camera );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_editor_camera );

  crude_scene_components_import( world, manager );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_editor_camera_update_system_, crude_ecs_on_editor_update, ctx, {
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_editor_camera ) },
  } );
}