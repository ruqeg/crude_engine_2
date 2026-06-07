#include <engine/core/assert.h>

#include <engine/scene/scene_ecs.h>
#include <engine/graphics/imgui.h>
#include <engine/physics/physics_ecs.h>
#include <engine/scene/node_manager.h>
#include <engine/graphics/scene_renderer.h>

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
ECS_COMPONENT_DECLARE( crude_ray );
ECS_COMPONENT_DECLARE( crude_ddgi_area );
ECS_COMPONENT_DECLARE( crude_terrain );
ECS_COMPONENT_DECLARE( crude_world_environment );

CRUDE_COMPONENT_STRING_DEFINE( crude_camera, "crude_camera" );
CRUDE_COMPONENT_STRING_DEFINE( crude_transform, "crude_transform" );
CRUDE_COMPONENT_STRING_DEFINE( crude_gltf, "crude_gltf" );
CRUDE_COMPONENT_STRING_DEFINE( crude_light, "crude_light" );
CRUDE_COMPONENT_STRING_DEFINE( crude_node_external, "crude_node_external" );
CRUDE_COMPONENT_STRING_DEFINE( crude_ray, "crude_ray" );
CRUDE_COMPONENT_STRING_DEFINE( crude_ddgi_area, "crude_ddgi_area" );
CRUDE_COMPONENT_STRING_DEFINE( crude_terrain, "crude_terrain" );
CRUDE_COMPONENT_STRING_DEFINE( crude_world_environment, "crude_world_environment" );

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
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_ray );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_ddgi_area );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_terrain );
  CRUDE_ECS_COMPONENT_DEFINE( world, crude_world_environment );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_transform );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_light );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_camera );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_gltf );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_node_external );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_ray );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_ddgi_area );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_terrain );
  CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DEFINE( manager, crude_world_environment );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_transform );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_light );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_camera );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_gltf );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_ray );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_ddgi_area );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_terrain );
  CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DEFINE( manager, crude_world_environment );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_transform );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_light );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_camera );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_gltf );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_ray );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_ddgi_area );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_terrain );
  CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DEFINE( manager, crude_world_environment );

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
    crude_gfx_model_renderer_resources_manager_get_gltf_model( manager->model_renderer_resources_manager, gltf_relative_filepath ) );

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

  CRUDE_IMGUI_OPTION( "Relative Filepath", {
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

          model_renderer_resources_handle = crude_gfx_model_renderer_resources_manager_get_gltf_model( manager->model_renderer_resources_manager, replace_relative_filepath );
          
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
  } );
  
#if CRUDE_DEVELOP
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
#endif /* CRUDE_DEVELOP */
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
    ImGui::ColorEdit3( "##Color", &component->color.x );
  } );
  CRUDE_IMGUI_OPTION( "Intensity", {
    ImGui::DragFloat( "##Intensity", &component->intensity );
  } );
  CRUDE_IMGUI_OPTION( "Radius", {
    ImGui::DragFloat( "##Radius", &component->radius );
  } );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_ray )
{
  crude_memory_set( component, 0, sizeof( crude_ray ) );
  component->distance = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "distance" ) );
  component->broad_phase_mask = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "broad_phase_mask" ) );
  component->layer_mask = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "layer_mask" ) );
  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_ray )
{
  cJSON *light_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( light_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_ray ) ) );
  cJSON_AddItemToObject( light_json, "distance", cJSON_CreateNumber( component->distance ) );
  cJSON_AddItemToObject( light_json, "broad_phase_mask", cJSON_CreateNumber( component->broad_phase_mask ) );
  cJSON_AddItemToObject( light_json, "layer_mask", cJSON_CreateNumber( component->layer_mask ) );
  return light_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_ray )
{
  CRUDE_IMGUI_START_OPTIONS;

  CRUDE_IMGUI_OPTION( "Distance", {
    ImGui::DragFloat( "##Distance", &component->distance );
  } );
  
  CRUDE_IMGUI_OPTION( "Broad Phase Mask", {
    ImGui::Spacing( );
    ImGui::CheckboxFlags( "Area", &component->broad_phase_mask, g_crude_jph_broad_phase_layer_area_mask );
    ImGui::SameLine( );
    ImGui::CheckboxFlags( "Static", &component->broad_phase_mask, g_crude_jph_broad_phase_layer_static_mask );
    ImGui::SameLine( );
    ImGui::CheckboxFlags( "Dynamic", &component->broad_phase_mask, g_crude_jph_broad_phase_layer_dynamic_mask );
    } );
  
  CRUDE_IMGUI_OPTION( "Layer Mask", {
    ImGui::Spacing( );
    ImGui::CheckboxFlags( "0", &component->layer_mask, g_crude_jph_mask_custom0 );
    ImGui::SameLine( );
    ImGui::CheckboxFlags( "1", &component->layer_mask, g_crude_jph_mask_custom1 );
    ImGui::SameLine( );
    ImGui::CheckboxFlags( "2", &component->layer_mask, g_crude_jph_mask_custom2 );
    ImGui::SameLine( );
    ImGui::CheckboxFlags( "3", &component->layer_mask, g_crude_jph_mask_custom3 );
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
    component->type = CRUDE_CAST( crude_node_external_type, component_type );
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
        CRUDE_ASSERT( false );
        //crude_entity extern_node = crude_node_copy_hierarchy( world, crude_node_manager_get_node( manager, replace_relative_filepath, world, false ), replace_relative_filepath, parent, true, true );
        //crude_entity_enable_hierarchy( world, extern_node, true );
        //crude_string_copy( component->node_relative_filepath, replace_relative_filepath, sizeof( component->node_relative_filepath ) );
      }
    }
    ImGui::EndDragDropTarget();
  }
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_ddgi_area )
{
  crude_memory_set( component, 0, sizeof( crude_ddgi_area ) );
  
  component->probe_spacing.x = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(component_json, "probe_spacing_x" ) );
  component->probe_spacing.y = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(component_json, "probe_spacing_y" ) );
  component->probe_spacing.z = cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(component_json, "probe_spacing_z" ) );
  component->max_probe_offset = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "max_probe_offset" ) );
  component->self_shadow_bias = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "self_shadow_bias" ) );
  component->hysteresis = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "hysteresis" ) );
  component->shadow_weight_power = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "shadow_weight_power" ) );
  component->infinite_bounces_multiplier = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "infinite_bounces_multiplier" ) );
  component->probe_update_per_frame = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "probe_update_per_frame" ) );
  component->offsets_calculations_count = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "offsets_calculations_count" ) );
  component->probe_rays = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "probe_rays" ) );
  component->probe_count.x = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "probe_count_x" ) );
  component->probe_count.y = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "probe_count_y" ) );
  component->probe_count.z = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "probe_count_z" ) );
  
  component->editor_probe_count = component->probe_count;

  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_ddgi_area )
{
  cJSON *ddgi_area_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( ddgi_area_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_ddgi_area ) ) );

  cJSON_AddItemToObject( ddgi_area_json, "probe_spacing_x", cJSON_CreateNumber( component->probe_spacing.x ) );
  cJSON_AddItemToObject( ddgi_area_json, "probe_spacing_y", cJSON_CreateNumber( component->probe_spacing.y ) );
  cJSON_AddItemToObject( ddgi_area_json, "probe_spacing_z", cJSON_CreateNumber( component->probe_spacing.z ) );
  cJSON_AddItemToObject( ddgi_area_json, "max_probe_offset", cJSON_CreateNumber( component->max_probe_offset )  );
  cJSON_AddItemToObject( ddgi_area_json, "self_shadow_bias", cJSON_CreateNumber( component->self_shadow_bias )  );
  cJSON_AddItemToObject( ddgi_area_json, "hysteresis", cJSON_CreateNumber( component->hysteresis ) );
  cJSON_AddItemToObject( ddgi_area_json, "shadow_weight_power", cJSON_CreateNumber( component->shadow_weight_power ) );
  cJSON_AddItemToObject( ddgi_area_json, "infinite_bounces_multiplier", cJSON_CreateNumber( component->infinite_bounces_multiplier ) );
  cJSON_AddItemToObject( ddgi_area_json, "probe_update_per_frame", cJSON_CreateNumber( component->probe_update_per_frame  ) );
  cJSON_AddItemToObject( ddgi_area_json, "offsets_calculations_count", cJSON_CreateNumber( component->offsets_calculations_count  ) );
  cJSON_AddItemToObject( ddgi_area_json, "probe_rays", cJSON_CreateNumber( component->probe_rays  ) );
  cJSON_AddItemToObject( ddgi_area_json, "probe_count_x", cJSON_CreateNumber( component->probe_count.x ) );
  cJSON_AddItemToObject( ddgi_area_json, "probe_count_y", cJSON_CreateNumber( component->probe_count.y ) );
  cJSON_AddItemToObject( ddgi_area_json, "probe_count_z", cJSON_CreateNumber( component->probe_count.z ) );
  return ddgi_area_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_ddgi_area )
{
  bool                                                     offsets_calculations_changed;

  CRUDE_IMGUI_START_OPTIONS;
  
  offsets_calculations_changed = false;

  CRUDE_IMGUI_OPTION( "Probe Rays", {
    ImGui::DragInt( "##Probe Rays", &component->probe_rays );
  } );

  CRUDE_IMGUI_OPTION( "Probe Count", {
    ImGui::DragInt3( "##Probe Count", &component->editor_probe_count.x );
    ImGui::SameLine( );
    if ( ImGui::Button( "Apply" ) )
    {
      component->probe_count = component->editor_probe_count;
    }
  } );

  CRUDE_IMGUI_OPTION( "Probe Spacing", {
    ImGui::DragFloat3( "##Probe Spacing", &component->probe_spacing.x );
  } );
  
  CRUDE_IMGUI_OPTION( "Max Probe Offset", {
    ImGui::DragFloat( "##Max Probe Offset", &component->max_probe_offset );
  } );
  
  CRUDE_IMGUI_OPTION( "Self Shadow Bias", {
    ImGui::DragFloat( "##Self Shadow Bias", &component->self_shadow_bias );
  } );
  
  CRUDE_IMGUI_OPTION( "Hysteresis", {
    ImGui::SliderFloat( "##Hysteresis", &component->hysteresis, 0.0001, 1.0 );
  } );
  
  CRUDE_IMGUI_OPTION( "Shadow Weight Power", {
    ImGui::DragFloat( "##Shadow Weight Power", &component->shadow_weight_power );
  } );
  
  CRUDE_IMGUI_OPTION( "Infinite Bounces Multiplier", {
    ImGui::DragFloat( "##Infinite Bounces Multiplier", &component->infinite_bounces_multiplier );
  } );
  
  CRUDE_IMGUI_OPTION( "Probe Update Per Frame", {
    ImGui::DragInt( "##Probe Update Per Frame", &component->probe_update_per_frame );
  } );

  CRUDE_IMGUI_OPTION( "Offsets Calculations Count", {
    offsets_calculations_changed |= ImGui::DragInt( "##Offsets Calculations Count", &component->offsets_calculations_count );
  } );

  if ( offsets_calculations_changed )
  {
    crude_gfx_indirect_light_pass_on_offsets_reset( &manager->scene_renderer->indirect_light_pass );
  }
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_terrain )
{
  crude_gfx_texture_manager                               *texture_manager;
  char const                                              *height_texture_relative_filepath;

  crude_memory_set( component, 0, sizeof( crude_terrain ) );
  
  texture_manager = manager->model_renderer_resources_manager->texture_manager;
  height_texture_relative_filepath = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "height_texture" ) );
  if ( crude_string_cmp( height_texture_relative_filepath, "none" ) != 0 )
  {
    component->height_texture_handle = crude_gfx_texture_manager_get_texture( texture_manager, height_texture_relative_filepath );
  }

  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_terrain )
{
  cJSON                                                   *terrain_json;
  crude_gfx_texture                                       *height_texture;

  terrain_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( terrain_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_terrain ) ) );

  height_texture = crude_gfx_access_texture( manager->model_renderer_resources_manager->gpu, component->height_texture_handle );
  cJSON_AddItemToObject( terrain_json, "height_texture", cJSON_CreateString( height_texture ? height_texture->name : "none" ) );
  
  return terrain_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_terrain )
{
  crude_gfx_texture                                       *heigth_texture;
  
  CRUDE_IMGUI_START_OPTIONS;

  heigth_texture = crude_gfx_access_texture( manager->model_renderer_resources_manager->gpu, component->height_texture_handle );

  CRUDE_IMGUI_OPTION( "Height Texture", {
    ImGui::Text( "\"%s\"", heigth_texture ? heigth_texture->name : "Empty" );
    if ( ImGui::BeginDragDropTarget( ) )
    {
      ImGuiPayload const                                  *im_payload;
      char                                                *replace_relative_filepath;
    
      im_payload = ImGui::AcceptDragDropPayload( "crude_content_browser_file" );
      if ( im_payload )
      {
        replace_relative_filepath = CRUDE_CAST( char*, im_payload->Data );
        if ( strstr( replace_relative_filepath, ".png" ) )
        {
          component->height_texture_handle = crude_gfx_texture_manager_get_texture( manager->model_renderer_resources_manager->texture_manager, replace_relative_filepath );
        }
      }
      ImGui::EndDragDropTarget();
    }
  } );
}

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_IMPLEMENTATION( crude_world_environment )
{
  crude_memory_set( component, 0, sizeof( crude_world_environment ) );
  
  component->background_color.x = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive(component_json, "background_color_x" ) );
  component->background_color.y = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive(component_json, "background_color_y" ) );
  component->background_color.z = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive(component_json, "background_color_z" ) );
  component->background_intencity = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive(component_json, "background_intencity" ) );

  return true;
}

CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_IMPLEMENTATION( crude_world_environment )
{
  cJSON *world_environment_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( world_environment_json, "type", cJSON_CreateString( CRUDE_COMPONENT_STRING( crude_world_environment ) ) );

  cJSON_AddItemToObject( world_environment_json, "background_color_x", cJSON_CreateNumber( component->background_color.x ) );
  cJSON_AddItemToObject( world_environment_json, "background_color_y", cJSON_CreateNumber( component->background_color.y ) );
  cJSON_AddItemToObject( world_environment_json, "background_color_z", cJSON_CreateNumber( component->background_color.z ) );
  cJSON_AddItemToObject( world_environment_json, "background_intencity", cJSON_CreateNumber( component->background_intencity ) );
  return world_environment_json;
}

CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_IMPLEMENTATION( crude_world_environment )
{
  CRUDE_IMGUI_START_OPTIONS;

  CRUDE_IMGUI_OPTION( "Background Color", {
    ImGui::ColorEdit3( "##Background Color", &component->background_color.x );
  } );

  CRUDE_IMGUI_OPTION( "Background Intencity", {
    ImGui::DragFloat( "##Background Intencity", &component->background_intencity );
  } );
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