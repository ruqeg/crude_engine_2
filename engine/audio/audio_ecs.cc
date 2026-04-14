#include <engine/scene/scene_ecs.h>
#include <engine/graphics/imgui.h>
#include <engine/scene/node_manager.h>
#include <engine/core/profiler.h>

#include <engine/audio/audio_ecs.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/

ECS_COMPONENT_DECLARE( crude_audio_listener );
ECS_COMPONENT_DECLARE( crude_audio_player );
ECS_COMPONENT_DECLARE( crude_audio_player_handle );

CRUDE_COMPONENT_STRING_DEFINE( crude_audio_listener, "crude_audio_listener" );
CRUDE_COMPONENT_STRING_DEFINE( crude_audio_player, "crude_audio_player" );
CRUDE_COMPONENT_STRING_DEFINE( crude_audio_player_handle, "crude_audio_player_handle" );

void
crude_audio_components_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager
)
{
  CRUDE_ECS_MODULE( world, crude_audio_components );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_audio_listener );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_audio_player );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_audio_player_handle );
  
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_audio_listener );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_audio_listener );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_audio_player );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_audio_player );
  
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_audio_listener );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_audio_player );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_audio_player_handle );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_audio_listener )
{
  crude_memory_set( component, 0, sizeof( crude_audio_listener ) );;
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_audio_listener )
{
  cJSON *camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_audio_listener ) ) );
  return camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_audio_listener )
{
  CRUDE_IMGUI_START_OPTIONS;
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_audio_player )
{
  crude_memory_set( component, 0, sizeof( crude_audio_player ) );
  crude_string_copy( component->relative_filepath, cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "path" ) ), sizeof( component->relative_filepath ) );
  component->looping = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "looping" ) );
  component->positioning = CRUDE_CAST( crude_audio_sound_positioning, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "positioning" ) ) );
  component->stream = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "stream" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_audio_player )
{
  cJSON *component_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( component_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_audio_player ) ) );
  cJSON_AddItemToObject( component_json, "path", cJSON_CreateString( component->relative_filepath ) );
  cJSON_AddItemToObject( component_json, "looping", cJSON_CreateNumber( component->looping ) );
  cJSON_AddItemToObject( component_json, "positioning", cJSON_CreateNumber( component->positioning ) );
  cJSON_AddItemToObject( component_json, "stream", cJSON_CreateNumber( component->stream ) );
  return component_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_audio_player )
{
  bool                                                     modified;

  CRUDE_IMGUI_START_OPTIONS;
  
  modified = false;

  CRUDE_IMGUI_OPTION( "Relative Filepath", {
    ImGui::Text( "\"%s\"", component->relative_filepath[ 0 ] ? component->relative_filepath : "Empty" );
    if ( ImGui::BeginDragDropTarget( ) )
    {
      ImGuiPayload const                                  *im_payload;
      char                                                *replace_relative_filepath;
    
      im_payload = ImGui::AcceptDragDropPayload( "crude_content_browser_file" );
      if ( im_payload )
      {
        replace_relative_filepath = CRUDE_CAST( char*, im_payload->Data );
        if ( strstr( replace_relative_filepath, ".mp3" ) )
        {
          modified = true;
          crude_string_copy( component->relative_filepath, replace_relative_filepath, sizeof( component->relative_filepath ) );
        }
      }
      ImGui::EndDragDropTarget();
    }
    } );
  
  
  CRUDE_IMGUI_OPTION( "Looping", {
    modified |= ImGui::Checkbox( "##Looping", &component->looping );
    } );

  CRUDE_IMGUI_OPTION( "Stream", {
    modified |= ImGui::Checkbox( "##Stream", &component->stream );
    } );
  
  char const *positioning_items[ 2 ] = 
  {
    "Absolute",
    "Realtive"
  };
  CRUDE_IMGUI_OPTION( "Positioning", {
    int32 current_item = component->positioning;
    modified |= ImGui::Combo( "##Positioning", &current_item, positioning_items, CRUDE_COUNTOF( positioning_items ) );
    component->positioning = CRUDE_CAST( crude_audio_sound_positioning, current_item );
    } );

  if ( modified )
  {
    CRUDE_ENTITY_SET_COMPONENT( world, node, crude_audio_player, { *component } );
  }
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_audio_player_handle )
{
  CRUDE_IMGUI_START_OPTIONS;
  
  if ( component->sound_handle.index != CRUDE_SOUND_HANDLE_INVALID.index )
  {
    CRUDE_IMGUI_OPTION( "Play", {
      if (ImGui::Button( "##Play" ) )
      { 
        crude_audio_device_sound_start( manager->audio_device, component->sound_handle );
      }
      });
    CRUDE_IMGUI_OPTION( "Stop", {
      if (ImGui::Button( "##Stop" ) )
      {
        crude_audio_device_sound_stop( manager->audio_device, component->sound_handle );
      }
      }); 
    CRUDE_IMGUI_OPTION( "Reset", {
      if (ImGui::Button( "##Reset" ) )
      {
        crude_audio_device_sound_reset( manager->audio_device, component->sound_handle );
      }
      }); 
    CRUDE_IMGUI_OPTION( "Volume", {
      float32 volume = crude_audio_device_sound_get_volume( manager->audio_device, component->sound_handle );
      if ( ImGui::DragFloat( "##Volume", &volume, 0.1f, 0.01f ) )
      {
        crude_audio_device_sound_set_volume( manager->audio_device, component->sound_handle, volume );
      }
      }); 
  }
}

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
CRUDE_ECS_SYSTEM_DECLARE( crude_audio_system );

CRUDE_ECS_SYSTEM_DECLARE( crude_audio_listener_update_system_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_audio_player_create_observer_ );
CRUDE_ECS_OBSERVER_DECLARE( crude_audio_player_destroy_observer_ );

void
crude_audio_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_audio_system_context                         *ctx
)
{
  crude_audio_components_import( world, manager );
  crude_scene_components_import( world, manager );

  CRUDE_ECS_SYSTEM_DEFINE( world, crude_audio_listener_update_system_, crude_ecs_on_engine_update, ctx, { 
    { .id = ecs_id( crude_transform ) },
    { .id = ecs_id( crude_audio_listener ) },
  } );
  
  CRUDE_ECS_OBSERVER_DEFINE( world, crude_audio_player_create_observer_, EcsOnSet, ctx, { 
    { .id = ecs_id( crude_audio_player ), .oper = EcsAnd }
  } );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_audio_player_destroy_observer_, EcsOnRemove, ctx, { 
    { .id = ecs_id( crude_audio_player_handle ), .oper = EcsAnd }
  } );
}

void
crude_audio_listener_update_system_ 
(
  _In_ ecs_iter_t                                         *it
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
crude_audio_player_create_observer_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_audio_system_context                              *ctx;
  crude_audio_player                                      *audio_player_per_entity;

  ctx = CRUDE_CAST( crude_audio_system_context*, it->ctx );
  audio_player_per_entity = ecs_field( it, crude_audio_player, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_audio_player                                    *audio_player;
    crude_entity                                           audio_player_node;
    
    audio_player = &audio_player_per_entity[ i ];
    audio_player_node = crude_entity_from_iterator( it, i );
    
    if ( audio_player->relative_filepath[ 0 ] )
    {
      crude_sound_creation                                 sound_creation;
      crude_audio_player_handle                            audio_player_handle;

      if ( CRUDE_ENTITY_HAS_COMPONENT( it->world, audio_player_node, crude_audio_player_handle ) )
      {
        crude_audio_device_destroy_sound( ctx->device, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( it->world, audio_player_node, crude_audio_player_handle )->sound_handle );
      }

      sound_creation = crude_sound_creation_empty( );
      sound_creation.looping = audio_player->looping;
      sound_creation.stream = audio_player->stream;
      sound_creation.decode = true;
      crude_string_copy( sound_creation.relative_filepath, audio_player->relative_filepath, sizeof( sound_creation.relative_filepath ) );
      sound_creation.positioning = audio_player->positioning;
      sound_creation.async_loading = false;

      audio_player_handle.sound_handle = crude_audio_device_create_sound( ctx->device, &sound_creation );
      CRUDE_ENTITY_SET_COMPONENT( it->world, audio_player_node, crude_audio_player_handle, { audio_player_handle } );
    }
  }
}

void
crude_audio_player_destroy_observer_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_audio_system_context                              *ctx;
  crude_audio_player_handle                               *audio_player_handle_per_entity;

  ctx = CRUDE_CAST( crude_audio_system_context*, it->ctx );
  audio_player_handle_per_entity = ecs_field( it, crude_audio_player_handle, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_audio_player_handle                             *audio_player_handle;
    crude_entity                                           audio_player_node;
    
    audio_player_handle = &audio_player_handle_per_entity[ i ];
    audio_player_node = crude_entity_from_iterator( it, i );
    
    crude_audio_device_destroy_sound( ctx->device, audio_player_handle->sound_handle );
  }
}