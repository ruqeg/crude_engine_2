#include <cJSON.h>

#include <core/assert.h>
#include <core/file.h>
#include <core/string.h>
#include <core/array.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <physics/physics_components.h>
#include <physics/physics_components.h>

#include <scene/scene.h>

static cJSON*
scene_parse_json_
(
  _In_ crude_scene                                        *scene,
  _In_ char const                                         *filepath
);

static crude_entity
scene_load_node_
(
  _In_ crude_scene                                        *scene,
  _In_ cJSON                                              *node_json
);

void
crude_scene_initialize
(
  _In_ crude_scene                                        *scene,
  _In_ crude_scene_creation const                         *creation
)
{
  cJSON                                                   *scene_json;
  char                                                     working_directory[ 512 ];
  size_t                                                   temporary_allocated_marker;
  crude_string_buffer                                      temporary_path_buffer;

  scene->allocator_container = creation->allocator_container;
  scene->temporary_allocator = creation->temporary_allocator;
  scene->input_entity = creation->input_entity;
  scene->world = creation->world;
  scene->additional_parse_json_to_component_func = creation->additional_parse_json_to_component_func;
  scene->additional_parse_all_components_to_json_func = creation->additional_parse_all_components_to_json_func;
  
  temporary_allocated_marker = crude_stack_allocator_get_marker( scene->temporary_allocator );

  crude_string_buffer_initialize( &scene->path_bufffer, 512, scene->allocator_container );
  crude_string_buffer_initialize( &temporary_path_buffer, 1024, crude_stack_allocator_pack( scene->temporary_allocator ) );
 
  crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
  scene->resources_path = crude_string_buffer_append_use_f( &scene->path_bufffer, "%s%s", working_directory, CRUDE_RESOURCES_DIR );

  scene_json = scene_parse_json_( scene, creation->filepath );
  if ( scene_json == NULL )
  {
    return;
  }
  scene->main_node = scene_load_node_( scene, scene_json );

  cJSON_Delete( scene_json );
  crude_stack_allocator_free_marker( scene->temporary_allocator, temporary_allocated_marker );
}

void
crude_scene_deinitialize
(
  _In_ crude_scene                                        *scene
)
{
  crude_string_buffer_deinitialize( &scene->path_bufffer );
}

cJSON*
node_to_json_hierarchy_
(
  _In_ crude_scene                                        *scene,
  _In_ crude_entity                                        node
)
{
  cJSON                                                   *node_json;
  bool                                                     is_external_node, is_gltf_node;
  
  is_external_node = is_gltf_node = false;

  node_json = cJSON_CreateObject( );
  
  cJSON_AddItemToObject( node_json, "name", cJSON_CreateString( crude_entity_get_name( node ) ) );

  is_external_node = CRUDE_ENTITY_HAS_COMPONENT( node, crude_node_external );
  if ( is_external_node )
  {
    cJSON_AddItemToObject( node_json, "external", cJSON_CreateString( CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_node_external )->path ) );
  }

  {
    crude_transform const                                 *node_transform;
    node_transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_transform );

    if ( node_transform )
    {
      cJSON                                               *node_transform_json;

      node_transform_json = cJSON_CreateObject( );
      cJSON_AddItemToObject( node_transform_json, "translation", cJSON_CreateFloatArray( &node_transform->translation.x, 3 ) );
      cJSON_AddItemToObject( node_transform_json, "rotation", cJSON_CreateFloatArray( &node_transform->rotation.x, 4 ) );
      cJSON_AddItemToObject( node_transform_json, "scale", cJSON_CreateFloatArray( &node_transform->scale.x, 3 ) );
      cJSON_AddItemToObject( node_json, "transform", node_transform_json );
    }
  }
  
  {
    cJSON                                                 *node_components_json;
    crude_free_camera const                               *node_free_camera;
    crude_camera const                                    *node_camera;
    crude_gltf const                                      *node_gltf;
    crude_light const                                     *node_light;
    crude_physics_static_body_handle const                *static_body;
    crude_physics_dynamic_body_handle const               *dynamic_body;
    crude_physics_collision_shape const                   *collision_shape;

    node_components_json = cJSON_AddArrayToObject( node_json, "components" );
    
    node_camera = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_camera );
    if ( node_camera )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_camera )( node_camera ) );
    }
    
    node_gltf = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_gltf );
    if ( node_gltf )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_gltf )( node_gltf ) );
      is_gltf_node = true;
    }
    
    node_free_camera = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_free_camera );
    if ( node_free_camera )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_free_camera )( node_free_camera ) );
    }

    node_light = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_light );
    if ( node_light )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_light )( node_light ) );
    }
    
    static_body = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_physics_static_body_handle );
    if ( static_body )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_physics_static_body_handle )( static_body ) );
    }
    
    dynamic_body = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_physics_dynamic_body_handle );
    if ( dynamic_body )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_physics_dynamic_body_handle )( dynamic_body ) );
    }
    
    collision_shape = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_physics_collision_shape );
    if ( collision_shape )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_physics_collision_shape )( collision_shape ) );
    }
    
    scene->additional_parse_all_components_to_json_func( node, node_components_json );
  }
  
  {
    cJSON                                                 *children_json;

    children_json = cJSON_AddArrayToObject( node_json, "children" );
    
    if ( !is_gltf_node && !is_external_node )
    {
      ecs_iter_t it = ecs_children( node.world, node.handle );
      while ( ecs_children_next( &it ) )
      {
        for ( size_t i = 0; i < it.count; ++i )
        {
          crude_entity                                       child;

          child = CRUDE_COMPOUNT( crude_entity, { .handle = it.entities[ i ], .world = node.world } );
          cJSON_AddItemToArray( children_json, node_to_json_hierarchy_( scene, child ) );
        }
      }
    }
  }

  return node_json;
}

void
crude_scene_save_to_file
(
  _In_ crude_scene                                        *scene,
  _In_ char const                                         *filename
)
{
  cJSON                                                   *scene_json;
  char const                                              *scene_str;

  scene_json = node_to_json_hierarchy_( scene, scene->main_node );
  
  scene_str = cJSON_Print( scene_json );
  crude_write_file( filename, scene_str, strlen( scene_str ) ); // TODO
  
  cJSON_Delete( scene_json );
}

crude_entity
scene_load_node_
(
  _In_ crude_scene                                        *scene,
  _In_ cJSON                                              *node_json
)
{
  crude_entity                                             node;
  char const                                              *node_name;
  bool                                                     is_node_external;
  
  node_name = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( node_json, "name") );

  // !TODO handle external with hashmap (we don't want to parse 100 entities of same node :D)
  is_node_external = cJSON_HasObjectItem( node_json, "external" );
  if ( is_node_external )
  {
    cJSON                                                 *node_external_json;
    char const                                            *node_external_json_filepath;
    uint64                                                 temporary_allocator_marker;
    crude_string_buffer                                    temporary_path_buffer;
    
    temporary_allocator_marker = crude_stack_allocator_get_marker( scene->temporary_allocator );
    crude_string_buffer_initialize( &temporary_path_buffer, 1024, crude_stack_allocator_pack( scene->temporary_allocator ) );

    node_external_json_filepath = cJSON_GetStringValue(  cJSON_GetObjectItemCaseSensitive( node_json, "external") );
    
    node_external_json = scene_parse_json_( scene, crude_string_buffer_append_use_f( &temporary_path_buffer, "%s%s", scene->resources_path, node_external_json_filepath ) );
    node = scene_load_node_( scene, node_external_json );

    CRUDE_ENTITY_SET_COMPONENT( node, crude_node_external, { crude_string_buffer_append_use_f( &scene->path_bufffer, "%s", node_external_json_filepath ) } );

    crude_stack_allocator_free_marker( scene->temporary_allocator, temporary_allocator_marker );
  }
  else
  {
    node = crude_entity_create_empty( scene->world, node_name );
  }

  {
    cJSON const                                           *node_transform_json;
    cJSON const                                           *node_components_json;
    cJSON const                                           *node_tags_json;
    crude_transform                                        transform;
  
    node_transform_json = cJSON_GetObjectItemCaseSensitive( node_json, "transform" );
    node_components_json = cJSON_GetObjectItemCaseSensitive( node_json, "components" );
    node_tags_json = cJSON_GetObjectItemCaseSensitive( node_json, "tags" );

    CRUDE_PARSE_JSON_TO_COMPONENT( crude_transform )( &transform, node_transform_json );
    CRUDE_ENTITY_SET_COMPONENT( node, crude_transform, { transform } );

    for ( uint32 component_index = 0; component_index < cJSON_GetArraySize( node_components_json ); ++component_index )
    {
      cJSON const                                       *component_json;
      cJSON const                                       *component_type_json;
      char const                                        *component_type;

      component_json = cJSON_GetArrayItem( node_components_json, component_index );
      component_type_json = cJSON_GetObjectItemCaseSensitive( component_json, "type" );
      component_type = cJSON_GetStringValue( component_type_json );
  
      if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_gltf ) ) == 0 )
      {
        crude_gltf                                         gltf;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_gltf )( &gltf, component_json );
        gltf.original_path = crude_string_buffer_append_use_f( &scene->path_bufffer, "%s", gltf.original_path );
        gltf.path = crude_string_buffer_append_use_f( &scene->path_bufffer, "%s%s", scene->resources_path, gltf.original_path );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_gltf, { gltf } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_camera ) ) == 0 )
      {
        crude_camera                                       camera;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_camera )( &camera, component_json );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_camera, { camera } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_free_camera ) ) == 0 )
      {
        crude_free_camera                                  free_camera;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_free_camera )( &free_camera, component_json );
        free_camera.entity_input = scene->input_entity;
        free_camera.enabled = false;
        CRUDE_ENTITY_SET_COMPONENT( node, crude_free_camera, { free_camera } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_light ) ) == 0 )
      {
        crude_light                                        light;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_light )( &light, component_json );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_light, { light } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_physics_static_body_handle ) ) == 0 )
      {
        crude_physics_static_body_handle                   static_body;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_physics_static_body_handle )( &static_body, component_json );
        CRUDE_ENTITY_REMOVE_COMPONENT( node, crude_physics_static_body_handle );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_static_body_handle, { static_body } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_physics_dynamic_body_handle ) ) == 0 )
      {
        crude_physics_dynamic_body_handle                  dynamic_body;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_physics_dynamic_body_handle )( &dynamic_body, component_json );
        CRUDE_ENTITY_REMOVE_COMPONENT( node, crude_physics_dynamic_body_handle );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_dynamic_body_handle, { dynamic_body } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_physics_collision_shape ) ) == 0 )
      {
        crude_physics_collision_shape                      collision_shape;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_physics_collision_shape )( &collision_shape, component_json );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_collision_shape, { collision_shape } );
      }
      else
      {
        scene->additional_parse_json_to_component_func( node, component_json, component_type );
      }
    }
  }
  
  if ( !is_node_external )
  {
    cJSON                                                 *children_json;
    cJSON                                                 *child_json;
  
    children_json = cJSON_GetObjectItemCaseSensitive( node_json, "children" );
    for ( uint32 child_index = 0; child_index < cJSON_GetArraySize( children_json ); ++child_index )
    {
      child_json = cJSON_GetArrayItem( children_json, child_index );
      crude_entity child_node = scene_load_node_( scene, child_json );
      crude_entity_set_parent( child_node, node );
    }
  }

  return node;
}

cJSON*
scene_parse_json_
(
  _In_ crude_scene                                        *scene,
  _In_ char const                                         *filepath
)
{
  cJSON                                                 *json;
  uint8                                                 *json_buffer;
  uint32                                                 json_buffer_size;
  
  if ( !crude_file_exist( filepath ) )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse scene", filepath );
    return NULL;
  }
  
  crude_read_file( filepath, crude_stack_allocator_pack( scene->temporary_allocator ), &json_buffer, &json_buffer_size );
  
  json = cJSON_ParseWithLength( CRUDE_REINTERPRET_CAST( char const*, json_buffer ), json_buffer_size );
  if ( !json )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file for scene... Error %s", cJSON_GetErrorPtr() );
    return NULL;
  }
}