#include <engine/core/assert.h>

#include <engine/scene/scene_ecs.h>
#include <engine/graphics/imgui.h>
#include <engine/physics/physics_ecs.h>
#include <engine/scene/node_manager.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/

CRUDE_ECS_OBSERVER_DECLARE( crude_gltf_destroy_observer_ );

static void
crude_gltf_destroy_observer_ 
(
  _In_ ecs_iter_t                                         *it
);

ECS_COMPONENT_DECLARE( crude_transform );
ECS_COMPONENT_DECLARE( crude_light );
ECS_COMPONENT_DECLARE( crude_camera );
ECS_COMPONENT_DECLARE( crude_gltf );
ECS_COMPONENT_DECLARE( crude_node_external );

CRUDE_COMPONENT_STRING_DEFINE( crude_camera, "crude_camera" );
CRUDE_COMPONENT_STRING_DEFINE( crude_transform, "crude_transform" );
CRUDE_COMPONENT_STRING_DEFINE( crude_gltf, "crude_gltf" );
CRUDE_COMPONENT_STRING_DEFINE( crude_light, "crude_light" );
CRUDE_COMPONENT_STRING_DEFINE( crude_node_external, "crude_node_external" );

void
crude_scene_components_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager
)
{
  CRUDE_ECS_MODULE( world, crude_scene_components );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_transform );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_light );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_camera );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_gltf );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_node_external );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_transform );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_light );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_camera );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_gltf );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_node_external );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_transform );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_light );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_camera );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_gltf );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_transform );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_light );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_camera );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_gltf );

  CRUDE_ECS_OBSERVER_DEFINE( world, crude_gltf_destroy_observer_, EcsOnRemove, NULL, { 
    { .id = ecs_id( crude_gltf ), .oper = EcsAnd }
  } );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_camera )
{
  crude_memory_set( component, 0, sizeof( crude_camera ) );
  component->fov_radians = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "fov_radians" ) ) );
  component->near_z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "near_z" ) ) );
  component->far_z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "far_z" ) ) );
  component->aspect_ratio = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "aspect_ratio" ) ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_camera )
{
  cJSON *camera_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( camera_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_camera ) ) );
  cJSON_AddItemToObject( camera_json, "fov_radians", cJSON_CreateNumber( component->fov_radians ) );
  cJSON_AddItemToObject( camera_json, "near_z", cJSON_CreateNumber( component->near_z ) );
  cJSON_AddItemToObject( camera_json, "far_z", cJSON_CreateNumber( component->far_z ) );
  cJSON_AddItemToObject( camera_json, "aspect_ratio", cJSON_CreateNumber( component->aspect_ratio ) );
  return camera_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_camera )
{
  CRUDE_IMGUI_START_OPTIONS;
  CRUDE_IMGUI_OPTION( "Aspect Ratio", {
    ImGui::DragFloat( "##Aspect Ratio", &component->aspect_ratio );
  } );
  CRUDE_IMGUI_OPTION( "Fov", {
    ImGui::SliderAngle( "##Fov", &component->fov_radians, 30.f );
  } );
  CRUDE_IMGUI_OPTION( "Near Plane", {
    ImGui::DragFloat( "##Near Plane", &component->near_z );
  } );
  CRUDE_IMGUI_OPTION( "Far Plane", {
    ImGui::DragFloat( "##Far Plane", &component->far_z );
  } );
  CRUDE_IMGUI_OPTION( "Select", {
    if ( ImGui::Button( "##Select" ) )
    {
      manager->select_camera_func( manager->select_camera_ctx, node );
    }
  } );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_transform )
{
  crude_memory_set( component, 0, sizeof( crude_camera ) );
  crude_parse_json_to_float3( &component->translation, cJSON_GetObjectItemCaseSensitive( component_json, "translation" ) );
  crude_parse_json_to_float4( &component->rotation, cJSON_GetObjectItemCaseSensitive( component_json, "rotation" ) );
  crude_parse_json_to_float3( &component->scale, cJSON_GetObjectItemCaseSensitive( component_json, "scale" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_transform )
{
  cJSON *transform_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( transform_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_transform ) ) );
  cJSON_AddItemToObject( transform_json, "translation", cJSON_CreateFloatArray( &component->translation.x, 3 ) );
  cJSON_AddItemToObject( transform_json, "rotation", cJSON_CreateFloatArray( &component->rotation.x, 4 ) );
  cJSON_AddItemToObject( transform_json, "scale", cJSON_CreateFloatArray( &component->scale.x, 3 ) );
  return transform_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_transform )
{
  XMFLOAT3                                               pitch_yaw_roll;

  CRUDE_IMGUI_START_OPTIONS;
  CRUDE_IMGUI_OPTION( "Translation", {
    ImGui::DragFloat3( "##Translation", &component->translation.x, 0.1f, 0.f, 0.f, "%.3f", ImGuiSliderFlags_ColorMarkers );
  } );
  
  CRUDE_IMGUI_OPTION( "Quaternion Rotation", {
    XMStoreFloat3( &pitch_yaw_roll, crude_quaternion_to_pitch_yaw_roll( XMLoadFloat4( &component->rotation ) ) );
    ImGui::DragFloat4( "##Quaternion Rotation", &component->rotation.x, 0.05f, 0.f, 0.f, "%.3f", ImGuiSliderFlags_ColorMarkers );
  } );

  CRUDE_IMGUI_OPTION( "Scale", {
    ImGui::DragFloat3( "##Scale", &component->scale.x, 0.1f, 0.f, 0.f, "%.3f", ImGuiSliderFlags_ColorMarkers );
  } );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_gltf )
{
  char const                                              *gltf_relative_filepath;
  crude_gfx_model_renderer_resources                      *model_renderer_resources;

  gltf_relative_filepath = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "path" ) );

  *component = CRUDE_COMPOUNT_EMPTY( crude_gltf );
  
  crude_gfx_model_renderer_resources_instance_initialize(
    &component->model_renderer_resources_instance,
    manager->model_renderer_resources_manager,
    crude_gfx_model_renderer_resources_manager_get_gltf_model( manager->model_renderer_resources_manager, gltf_relative_filepath, NULL ) );

  component->hidden = cJSON_HasObjectItem( component_json, "hidden" ) ? cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "hidden" ) ) : false;
  
  {
    cJSON const                                           *animations_instances_json;

    animations_instances_json = cJSON_GetObjectItemCaseSensitive( component_json, "animations_instances" );
    if ( animations_instances_json )
    {
      for ( uint32 i = 0; i < cJSON_GetArraySize( animations_instances_json ); ++i )
      {
        crude_gfx_model_renderer_resources_animation_instance *animation_instance;
        cJSON const                                       *animation_instance_json;
        
        animation_instance_json = cJSON_GetArrayItem( animations_instances_json, i );

        animation_instance = &component->model_renderer_resources_instance.animations_instances[ i ];
        animation_instance->animation_index = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( animation_instance_json, "index" ) );
        for ( uint32 k = 0; k < CRUDE_COUNTOF( animation_instance->nodes_enabled_bits ); ++k )
        {
          animation_instance->nodes_enabled_bits[ k ] = cJSON_GetNumberValue( cJSON_GetArrayItem( cJSON_GetObjectItemCaseSensitive( animation_instance_json, "nodes_enabled_bits" ), k ) );
        }
      }
    }
  }

  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_gltf )
{
  cJSON                                                   *gltf_json;
  cJSON                                                   *animations_instances_array_json;
  crude_gfx_model_renderer_resources                      *model_renderer_resources;

  CRUDE_ASSERT( component->model_renderer_resources_instance.model_renderer_resources_handle.index != -1 );

  model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( manager->model_renderer_resources_manager, component->model_renderer_resources_instance.model_renderer_resources_handle );

  gltf_json = cJSON_CreateObject( );     
  cJSON_AddItemToObject( gltf_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_gltf ) ) );
  cJSON_AddItemToObject( gltf_json, "path", cJSON_CreateString( model_renderer_resources->relative_filepath ) );
  if ( component->hidden )
  {
    cJSON_AddItemToObject( gltf_json, "hidden", cJSON_CreateBool( component->hidden ) );
  }

  animations_instances_array_json = cJSON_CreateArray( );
  for ( uint32 i = 0; i < CRUDE_COUNTOF( component->model_renderer_resources_instance.animations_instances ); ++i )
  {
    crude_gfx_model_renderer_resources_animation_instance const *animation_instance;
    cJSON                                                 *animation_instance_json;

    animation_instance = &component->model_renderer_resources_instance.animations_instances[ i ];
    animation_instance_json = cJSON_CreateObject( );
    cJSON_AddItemToObject( animation_instance_json, "index", cJSON_CreateNumber( animation_instance->animation_index ) );
    cJSON_AddItemToObject( animation_instance_json, "nodes_enabled_bits", cJSON_CreateIntArray( animation_instance->nodes_enabled_bits, CRUDE_COUNTOF( animation_instance->nodes_enabled_bits ) ) );
    cJSON_AddItemToArray( animations_instances_array_json, animation_instance_json );
  }
  cJSON_AddItemToObject( gltf_json, "animations_instances", animations_instances_array_json );
  return gltf_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_gltf )
{
  crude_gfx_model_renderer_resources                      *model_renderer_resources;

  CRUDE_IMGUI_START_OPTIONS;

  model_renderer_resources = crude_gfx_model_renderer_resources_manager_access_model_renderer_resources( manager->model_renderer_resources_manager, component->model_renderer_resources_instance.model_renderer_resources_handle );

  CRUDE_IMGUI_OPTION( "Hidden", {
    ImGui::Checkbox( "##Hidden", &component->hidden );
  } );
  
  CRUDE_IMGUI_OPTION( "Cast Shadow", {
    ImGui::Checkbox( "##Cast Shadow", &component->model_renderer_resources_instance.cast_shadow );
  } );
  

  ImGui::Text( "\"%s\"", model_renderer_resources ? model_renderer_resources->relative_filepath : "Empty" );
  if ( ImGui::BeginDragDropTarget( ) )
  {
    ImGuiPayload const                                  *im_payload;
    char                                                *replace_relative_filepath;
  
    im_payload = ImGui::AcceptDragDropPayload( "crude_content_browser_file" );
    if ( im_payload )
    {
      replace_relative_filepath = CRUDE_CAST( char*, im_payload->Data );
      if ( strstr( replace_relative_filepath, ".gltf" ) )
      {
        crude_gfx_model_renderer_resources              *model_renderer_resources;
        crude_gfx_model_renderer_resources_handle        model_renderer_resources_handle;

        model_renderer_resources_handle = crude_gfx_model_renderer_resources_manager_get_gltf_model( manager->model_renderer_resources_manager, replace_relative_filepath, NULL );
        
        if ( component->model_renderer_resources_instance.model_renderer_resources_handle.index != -1 )
        {
          crude_gfx_model_renderer_resources_instance_deinitialize( &component->model_renderer_resources_instance );
        }
  
        crude_gfx_model_renderer_resources_instance_initialize(
          &component->model_renderer_resources_instance,
          manager->model_renderer_resources_manager,
          model_renderer_resources_handle );
      }
    }
    ImGui::EndDragDropTarget();
  }

  CRUDE_IMGUI_OPTION( "Relative Filepath", {
  } );
  
  CRUDE_IMGUI_OPTION( "Animations", {
    if ( model_renderer_resources )
    {
      crude_gfx_animation                                 *animations;
      crude_gfx_model_renderer_resources_animation_instance *animation_instance;
      
      animation_instance = &component->model_renderer_resources_instance.animations_instances[ component->debug_animation_instance_index ];
      animations = model_renderer_resources->animations;

      ImGui::Spacing( );

      ImGui::SliderInt( "Animation Instance", &component->debug_animation_instance_index, 0, 7 );
      
      if ( ImGui::BeginCombo( "Animation", ( animation_instance->animation_index != -1 ) ? animations[ animation_instance->animation_index ].name : "None", ImGuiComboFlags_WidthFitPreview ) )
      {
        for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( animations ); i++ )
        {
          bool is_selected = ( animation_instance->animation_index == i );
          if ( ImGui::Selectable( animations[ i ].name, is_selected ) )
          {
            animation_instance->animation_index = i;
          }
          
          if ( is_selected )
          {
            ImGui::SetItemDefaultFocus( );
          }
        }
        ImGui::EndCombo( );
      }

      if ( ImGui::Button( "Reset" ) )
      {
        animation_instance->animation_index = -1;
      }
      
      if ( animation_instance->animation_index != -1 )
      {
        ImGui::InputText( "Name", animations[ animation_instance->animation_index ].name, 256, ImGuiInputTextFlags_ReadOnly );
      }
      else
      {
        ImGui::Text( "No Name" );
      }
      if ( animation_instance->animation_index != -1 )
      {
        ImGui::SliderFloat( "Current", &animation_instance->current_time, animations[ animation_instance->animation_index ].start, animations[ animation_instance->animation_index ].end );
      }
      ImGui::Checkbox( "Inverse", &animation_instance->inverse );
      ImGui::Checkbox( "Loop", &animation_instance->loop );
      ImGui::Checkbox( "Paused", &animation_instance->paused );
      ImGui::Checkbox( "Disabled", &animation_instance->disabled );
      ImGui::DragFloat( "Speed", &animation_instance->speed, 0.1f, 0.1f );
      if ( ImGui::CollapsingHeader( "Affected Nodes" ) )
      {
        for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model_renderer_resources->nodes ); ++i )
        {
          bool                                             node_enabled;
          
          ImGui::PushID( i );
          
          node_enabled = crude_gfx_model_renderer_resources_animation_instance_is_enabled_node( animation_instance, i );
          ImGui::Checkbox( "", &node_enabled );
          crude_gfx_model_renderer_resources_animation_instance_enable_node( animation_instance, i, node_enabled );

          ImGui::SameLine( );
          
          ImGui::Text( "%i | %s", i, model_renderer_resources->nodes[ i ].name );

          ImGui::PopID( );
        }
      }
    }
    } );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_light )
{
  crude_memory_set( component, 0, sizeof( crude_light ) );
  component->radius = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "radius" ) );
  crude_parse_json_to_float3( &component->color, cJSON_GetObjectItemCaseSensitive( component_json, "color" ) );
  component->intensity = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "intensity" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_light )
{
  cJSON *light_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( light_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_light ) ) );
  cJSON_AddItemToObject( light_json, "radius", cJSON_CreateNumber( component->radius ) );
  cJSON_AddItemToObject( light_json, "color", cJSON_CreateFloatArray( &component->color.x, 3 ) );
  cJSON_AddItemToObject( light_json, "intensity", cJSON_CreateNumber( component->intensity ) );
  return light_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_light )
{
  CRUDE_IMGUI_START_OPTIONS;

  CRUDE_IMGUI_OPTION( "Color", {
    ImGui::ColorPicker3( "##Color", &component->color.x );
  } );
  CRUDE_IMGUI_OPTION( "Intensity", {
    ImGui::DragFloat( "##Intensity", &component->intensity );
  } );
  CRUDE_IMGUI_OPTION( "Radius", {
    ImGui::DragFloat( "##Radius", &component->radius );
  } );
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_node_external )
{
  char const *node_external_names[ 2 ] = 
  {
    "Reference",
    "Copy"
  };
  CRUDE_ASSERT( CRUDE_COUNTOF( node_external_names ) == CRUDE_NODE_EXTERNAL_TYPE_COUNT );

  CRUDE_IMGUI_START_OPTIONS;
  
  CRUDE_IMGUI_OPTION( "Relative Filepath", {
    int32                                                  component_type;

    component_type = component->type;
    ImGui::Combo( "##Relative Filepath", &component_type, node_external_names, CRUDE_COUNTOF( node_external_names ) ); 
  } );
  
  ImGui::Text( "\"%s\"", component->node_relative_filepath[ 0 ] ? component->node_relative_filepath : "Empty" );

  if ( ImGui::BeginDragDropTarget( ) )
  {
    ImGuiPayload const                                  *im_payload;
    char                                                *replace_relative_filepath;
  
    im_payload = ImGui::AcceptDragDropPayload( "crude_content_browser_file" );
    if ( im_payload )
    {
      replace_relative_filepath = CRUDE_CAST( char*, im_payload->Data );
      if ( strstr( replace_relative_filepath, ".crude_node" ) )
      {
        crude_entity parent = crude_entity_get_parent( world, node );
        if ( component->node_relative_filepath[ 0 ] )
        {
          crude_entity_destroy_hierarchy( world, node );
        }
        crude_entity extern_node = crude_node_copy_hierarchy( world, crude_node_manager_get_node( manager, replace_relative_filepath, world, false ), replace_relative_filepath, parent, true, true );
        crude_entity_enable_hierarchy( world, extern_node, true );
        crude_string_copy( component->node_relative_filepath, replace_relative_filepath, sizeof( component->node_relative_filepath ) );
      }
    }
    ImGui::EndDragDropTarget();
  }
}

void
crude_gltf_destroy_observer_ 
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_gltf *gltf_per_entity = ecs_field( it, crude_gltf, 0 );

  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_gltf                                            *gltf;

    gltf = &gltf_per_entity[ i ];
    
    crude_gfx_model_renderer_resources_instance_deinitialize( &gltf->model_renderer_resources_instance );
  }
}