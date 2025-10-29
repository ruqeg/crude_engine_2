#include <cJSON.h>

#include <core/assert.h>
#include <core/file.h>
#include <core/string.h>
#include <core/array.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <physics/physics_components.h>

#include <scene/scene.h>

static XMFLOAT2
json_object_to_float2_
(
  cJSON                                                 *json
)
{
  CRUDE_ASSERT( cJSON_GetArraySize( json ) == 2 );

  XMFLOAT2 result;
  result.x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json , 0 ) ) );
  result.y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json , 1 ) ) );
  return result;
}

static XMFLOAT3
json_object_to_float3_
(
  cJSON                                                 *json
)
{
  CRUDE_ASSERT( cJSON_GetArraySize( json ) == 3 );
  
  XMFLOAT3 result;
  result.x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json , 0 ) ) );
  result.y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json , 1 ) ) );
  result.z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json , 2 ) ) );
  return result;
}

static XMFLOAT4
json_object_to_float4_
(
  cJSON                                                 *json
)
{
  CRUDE_ASSERT( cJSON_GetArraySize( json ) == 4 );
  XMFLOAT4 result;
  result.x = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json , 0 ) ) );
  result.y = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json , 1 ) ) );
  result.z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json , 2 ) ) );
  result.w = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetArrayItem( json , 3 ) ) );
  return result;
}

static crude_entity
scene_load_hierarchy_
(
  _In_ crude_scene                                        *scene,
  _In_ cJSON                                              *hierarchy_json
)
{
  crude_entity                                             node;
  
  {
    cJSON const                                           *node_json;
    char const                                            *node_name;
    cJSON const                                           *node_transform_json;
    cJSON const                                           *node_components_json;
    cJSON const                                           *node_tags_json;
  
    node_json = cJSON_GetObjectItemCaseSensitive( hierarchy_json, "node" ); 
    node_name = cJSON_GetStringValue(  cJSON_GetObjectItemCaseSensitive( node_json, "name") );
    node_transform_json = cJSON_GetObjectItemCaseSensitive( node_json, "transform" );
    node_components_json = cJSON_GetObjectItemCaseSensitive( node_json, "components" );
    node_tags_json = cJSON_GetObjectItemCaseSensitive( node_json, "tags" );
  
    node = crude_entity_create_empty( scene->world, node_name );
    
    CRUDE_ENTITY_SET_COMPONENT( node, crude_transform, {
      .translation = json_object_to_float3_( cJSON_GetObjectItemCaseSensitive( node_transform_json, "translation" ) ),
      .rotation    = json_object_to_float4_( cJSON_GetObjectItemCaseSensitive( node_transform_json, "rotation" ) ),
      .scale       = json_object_to_float3_( cJSON_GetObjectItemCaseSensitive( node_transform_json, "scale" ) ),
    } );
  
    for ( uint32 component_index = 0; component_index < cJSON_GetArraySize( node_components_json ); ++component_index )
    {
      cJSON const                                       *component_json;
      cJSON const                                       *component_type_json;
      char const                                        *component_type;
  
      component_json = cJSON_GetArrayItem( node_components_json, component_index );
      component_type_json = cJSON_GetObjectItemCaseSensitive( component_json, "type" );
      component_type = cJSON_GetStringValue( component_type_json );
  
      if ( crude_string_cmp( component_type, "crude_gltf" ) == 0 )
      {
        CRUDE_ENTITY_SET_COMPONENT( node, crude_gltf, {
          .path = crude_string_buffer_append_use_f( &scene->path_bufffer, "%s%s", scene->resources_path, cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( component_json, "path" ) ) )
        } );
      }
      else if ( crude_string_cmp( component_type, "crude_camera" ) == 0 )
      {
        CRUDE_ENTITY_SET_COMPONENT( node, crude_camera, {
          .fov_radians = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "fov_radians" ) ) ),
          .near_z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "near_z" ) ) ),
          .far_z = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "far_z" ) ) ),
          .aspect_ratio = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "aspect_ratio" ) ) ),
        } );
      }
      else if ( crude_string_cmp( component_type, "crude_free_camera" ) == 0 )
      {
        CRUDE_ENTITY_SET_COMPONENT( node, crude_free_camera, {
          .moving_speed_multiplier   = json_object_to_float3_( cJSON_GetObjectItemCaseSensitive( component_json, "moving_speed_multiplier" ) ),
          .rotating_speed_multiplier = json_object_to_float2_( cJSON_GetObjectItemCaseSensitive( component_json, "rotating_speed_multiplier" ) ),
          .entity_input              = scene->input_entity,
          .enabled                   = false
        } );
      }
      else if ( crude_string_cmp( component_type, "crude_light" ) == 0 )
      {
        CRUDE_ENTITY_SET_COMPONENT( node, crude_light, {
          .radius = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "radius" ) ) ),
          .color = json_object_to_float3_( cJSON_GetObjectItemCaseSensitive( component_json, "color" ) ),
          .intensity = CRUDE_STATIC_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( component_json, "intensity" ) ) ),
        } );
      }
      else if ( crude_string_cmp( component_type, "crude_physics_static_body" ) == 0 )
      {
        if ( cJSON_HasObjectItem( component_json, "crude_physics_box_collision_shape" ) )
        {
          cJSON *physics_box_collision_shape_json = cJSON_GetObjectItemCaseSensitive( component_json, "crude_physics_box_collision_shape" );
          CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_static_body, {
            .box_shape = 
            {
              .half_extent = json_object_to_float3_( cJSON_GetObjectItemCaseSensitive( physics_box_collision_shape_json, "half_extent" ) ),
            },
            .collision_shape_type = CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX
          } );
        }
        else
        {
          CRUDE_ASSERT( false );
        }
      }
      else if ( crude_string_cmp( component_type, "crude_physics_dynamic_body" ) == 0 )
      {
        if ( cJSON_HasObjectItem( component_json, "crude_physics_sphere_collision_shape" ) )
        {
          cJSON *physics_sphere_collision_shape_json = cJSON_GetObjectItemCaseSensitive( component_json, "crude_physics_sphere_collision_shape" );
          CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_dynamic_body, {
            .sphere_shape = 
            {
              .radius = CRUDE_CAST( float32, cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( physics_sphere_collision_shape_json, "radius" ) ) ),
            },
            .collision_shape_type = CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE
          } );
        }
        else
        {
          CRUDE_ASSERT( false );
        }
      }
    }

    for ( uint32 tag_index = 0; tag_index < cJSON_GetArraySize( node_tags_json ); ++tag_index )
    {
      cJSON const                                       *tag_json;
      char const                                        *tag;
  
      tag_json = cJSON_GetArrayItem( node_tags_json, tag_index );
      tag = cJSON_GetStringValue( tag_json );
  
      if ( crude_string_cmp( tag, "crude_editor_camera" ) == 0 )
      {
        scene->editor_camera_node = node;
      }
    }
  
    CRUDE_ARRAY_PUSH( scene->nodes, node );
  }
  
  {
    cJSON                                                 *hierarchy_children_json;
    cJSON                                                 *child_json;
  
    hierarchy_children_json = cJSON_GetObjectItemCaseSensitive( hierarchy_json, "children" );
    for ( uint32 child_index = 0; child_index < cJSON_GetArraySize( hierarchy_children_json ); ++child_index )
    {
      child_json = cJSON_GetArrayItem( hierarchy_children_json, child_index );
      crude_entity child_node = scene_load_hierarchy_( scene, child_json );
      crude_entity_set_parent( child_node, node );
    }
  }

  return node;
}

void
crude_scene_initialize
(
  _In_ crude_scene                                        *scene,
  _In_ crude_scene_creation const                         *creation
)
{
  cJSON                                                   *scene_json;
  size_t                                                   allocated_marker;
  crude_string_buffer                                      temporary_path_buffer;

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

  allocated_marker = crude_stack_allocator_get_marker( scene->temporary_allocator );
  
  crude_string_buffer_initialize( &temporary_path_buffer, 1024, crude_stack_allocator_pack( scene->temporary_allocator ) );
  
  /* Parse json */
  {
    char const                                            *json_path;
    uint8                                                 *json_buffer;
    uint32                                                 json_buffer_size;

    json_path = crude_string_buffer_append_use_f( &temporary_path_buffer, "%s%s", scene->resources_path, creation->filename );
    if ( !crude_file_exist( json_path ) )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse render graph", json_path );
      return;
    }

    crude_read_file( json_path, crude_stack_allocator_pack( scene->temporary_allocator ), &json_buffer, &json_buffer_size );

    scene_json = cJSON_ParseWithLength( CRUDE_REINTERPRET_CAST( char const*, json_buffer ), json_buffer_size );
    if ( !scene_json )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file for scene... Error %s", cJSON_GetErrorPtr() );
      return;
    }
  }

  /* Load hierarchy */
  {
    cJSON                                                 *hierarchy_json;
    hierarchy_json = cJSON_GetObjectItemCaseSensitive( scene_json, "hierarchy" );
    scene->main_node = scene_load_hierarchy_( scene, hierarchy_json );
  }

  cJSON_Delete( scene_json );
  crude_stack_allocator_free_marker( scene->temporary_allocator, allocated_marker );
}

void
crude_scene_deinitialize
(
  _In_ crude_scene                                        *scene,
  _In_ bool                                                destroy_nodes
)
{
  if ( destroy_nodes )
  {
    for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene->nodes ); ++i )
    {
      crude_entity_destroy( scene->nodes[ i ] );
    }
  }
  CRUDE_ARRAY_DEINITIALIZE( scene->nodes );
  crude_string_buffer_deinitialize( &scene->path_bufffer );
}