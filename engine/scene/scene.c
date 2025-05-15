#include <cJSON.h>

#include <core/assert.h>
#include <core/file.h>
#include <core/string.h>
#include <core/array.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>

#include <scene/scene.h>

static crude_float2
json_object_to_float2_
(
  cJSON                                                 *json
)
{
  CRUDE_ASSERT( cJSON_GetArraySize( json ) == 2 );

  crude_float2 result = {
    .x = cJSON_GetNumberValue( cJSON_GetArrayItem( json , 0 ) ),
    .y = cJSON_GetNumberValue( cJSON_GetArrayItem( json , 1 ) ),\
  };
  return result;
}

static crude_float3
json_object_to_float3_
(
  cJSON                                                 *json
)
{
  CRUDE_ASSERT( cJSON_GetArraySize( json ) == 3 );

  crude_float3 result = {
    .x = cJSON_GetNumberValue( cJSON_GetArrayItem( json , 0 ) ),
    .y = cJSON_GetNumberValue( cJSON_GetArrayItem( json , 1 ) ),
    .z = cJSON_GetNumberValue( cJSON_GetArrayItem( json , 2 ) )
  };
  return result;
}

static crude_float4
json_object_to_float4_
(
  cJSON                                                 *json
)
{
  CRUDE_ASSERT( cJSON_GetArraySize( json ) == 4 );
  crude_float4 result = {
    .x = cJSON_GetNumberValue( cJSON_GetArrayItem( json , 0 ) ),
    .y = cJSON_GetNumberValue( cJSON_GetArrayItem( json , 1 ) ),
    .z = cJSON_GetNumberValue( cJSON_GetArrayItem( json , 2 ) ),
    .w = cJSON_GetNumberValue( cJSON_GetArrayItem( json , 3 ) )
  };
  return result;
}

static void
load_scene_hierarchy_
(
  _In_ crude_scene                                        *scene,
  _In_ cJSON                                              *hierarchy_json
)
{
  cJSON                                                   *hierarchy_children_json;
  cJSON const                                             *node_json;
  size_t                                                   parent_node_index;
  
  parent_node_index = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( hierarchy_json, "node" ) ); 

  hierarchy_children_json = cJSON_GetObjectItemCaseSensitive( hierarchy_json, "children" );
  for ( uint32 child_index = 0; child_index < cJSON_GetArraySize( hierarchy_children_json ); ++child_index )
  {
    cJSON                                                 *child_json;
    size_t                                                 child_node_index;

    child_json = cJSON_GetArrayItem( hierarchy_children_json, child_index );
    child_node_index = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( child_json, "node" ) ); 
    crude_entity_set_parent( scene->nodes[ child_node_index ], scene->nodes[ parent_node_index ] );

    load_scene_hierarchy_( scene, child_json );
  }
}

void
crude_scene_initialize
(
  _In_ crude_scene                                        *scene,
  _In_ crude_scene_creation const                         *creation
)
{
  scene->allocator_container = creation->allocator_container;
  scene->temporary_allocator = creation->temporary_allocator;
  scene->input_entity = creation->input_entity;
  scene->world = creation->world;
  crude_string_buffer_initialize( &scene->path_bufffer, 512, scene->allocator_container );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene->nodes, 4, scene->allocator_container );

  {
    char working_directory[ 512 ];
    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    scene->resources_path = crude_string_buffer_append_use_f( &scene->path_bufffer, "%s%s", working_directory, creation->resources_path );
  }
}

void
crude_scene_deinitialize
(
  _In_ crude_scene                                        *scene
)
{
  CRUDE_ARRAY_DEINITIALIZE( scene->nodes );
  crude_string_buffer_deinitialize( &scene->path_bufffer );
}

void
crude_scene_load
(
  _In_ crude_scene                                        *scene,
  _In_ char const                                         *json_name
)
{
  cJSON                                                   *scene_json;
  size_t                                                   allocated_marker;
  crude_string_buffer                                      temporary_path_buffer;

  allocated_marker = crude_stack_allocator_get_marker( scene->temporary_allocator );
  
  crude_string_buffer_initialize( &temporary_path_buffer, 1024, crude_stack_allocator_pack( scene->temporary_allocator ) );
  
  /* Parse json */
  {
    char const                                            *json_path;
    uint8                                                 *json_buffer;
    size_t                                                 json_buffer_size;

    json_path = crude_string_buffer_append_use_f( &temporary_path_buffer, "%s%s", scene->resources_path, json_name );
    if ( !crude_file_exist( json_path ) )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse render graph", json_path );
      return;
    }

    crude_read_file( json_path, crude_stack_allocator_pack( scene->temporary_allocator ), &json_buffer, &json_buffer_size );

    scene_json = cJSON_ParseWithLength( json_buffer, json_buffer_size );
    if ( !scene_json )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file for scene... Error %s", cJSON_GetErrorPtr() );
      return;
    }
  }

  /* Load nodes */
  {
    cJSON                                                 *nodes_json;

    nodes_json = cJSON_GetObjectItemCaseSensitive( scene_json, "nodes" );
    CRUDE_ARRAY_SET_LENGTH( scene->nodes, cJSON_GetArraySize( nodes_json ) );
    for ( uint32 node_index = 0; node_index < cJSON_GetArraySize( nodes_json ); ++node_index )
    {
      cJSON const                                         *node_json;
      char const                                          *node_name;
      cJSON const                                         *node_transform_json;
      cJSON const                                         *node_components_json;

      node_json = cJSON_GetArrayItem( nodes_json, node_index );
      node_name = cJSON_GetStringValue(  cJSON_GetObjectItemCaseSensitive( node_json, "name") );
      node_transform_json = cJSON_GetObjectItemCaseSensitive( node_json, "transform" );
      node_components_json = cJSON_GetObjectItemCaseSensitive( node_json, "components" );

      crude_entity node = crude_entity_create_empty( scene->world, node_name );
      CRUDE_ENTITY_SET_COMPONENT( node, crude_transform, {
        .translation = json_object_to_float3_( cJSON_GetObjectItemCaseSensitive( node_transform_json, "translation" ) ),
        .scale       = json_object_to_float3_( cJSON_GetObjectItemCaseSensitive( node_transform_json, "scale" ) ),
        .rotation    = json_object_to_float4_( cJSON_GetObjectItemCaseSensitive( node_transform_json, "rotation" ) ),
      } );

      for ( uint32 component_index = 0; component_index < cJSON_GetArraySize( node_components_json ); ++component_index )
      {
        cJSON const                                       *component_json;
        cJSON const                                       *component_type_json;
        char const                                        *component_type;

        component_json = cJSON_GetArrayItem( node_components_json, component_index );
        component_type_json = cJSON_GetObjectItemCaseSensitive( component_json, "type" );
        component_type = cJSON_GetStringValue( component_type_json );

        if ( crude_string_cmp( component_type_json, "crude_gltf" ) == 0 )
        {
          CRUDE_ENTITY_SET_COMPONENT( node, crude_gltf, {
            .path = crude_string_buffer_append_use_f( &scene->path_bufffer, "%s%s", scene->resources_path, cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "path" ) ) )
          } );
        }
        else if ( crude_string_cmp( component_type_json, "crude_camera" ) == 0 )
        {
          CRUDE_ENTITY_SET_COMPONENT( node, crude_camera, {
            .fov_radians = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "fov_radians" ) ),
            .near_z = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "near_z" ) ),
            .far_z = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "far_z" ) ),
            .aspect_ratio = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "aspect_ratio" ) ),
          } );
        }
        else if ( crude_string_cmp( component_type_json, "crude_free_camera" ) == 0 )
        {
          CRUDE_ENTITY_SET_COMPONENT( node, crude_free_camera, {
            .moving_speed_multiplier   = json_object_to_float3_( cJSON_GetObjectItemCaseSensitive( component_json, "moving_speed_multiplier" ) ),
            .rotating_speed_multiplier = json_object_to_float2_( cJSON_GetObjectItemCaseSensitive( component_json, "rotating_speed_multiplier" ) ),
            .entity_input              = scene->input_entity,
          } );
        }
      }

      scene->nodes[ node_index ] = node;
    }
  }
  
  /* Load hierarchy */
  {
    cJSON                                                 *hierarchy_json;
    hierarchy_json = cJSON_GetObjectItemCaseSensitive( scene_json, "hierarchy" );
    load_scene_hierarchy_( scene, hierarchy_json );
  }

  cJSON_Delete( scene_json );
  crude_stack_allocator_free_marker( scene->temporary_allocator, allocated_marker );
}