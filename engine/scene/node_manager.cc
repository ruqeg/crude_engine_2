#include <cJSON.h>

#include <engine/core/assert.h>
#include <engine/core/file.h>
#include <engine/core/string.h>
#include <engine/core/array.h>
#include <engine/scene/scene_components.h>
#include <engine/scene/scripts_components.h>
#include <engine/core/hash_map.h>
#include <engine/physics/physics_components.h>
#include <engine/physics/physics_components.h>

#include <engine/scene/node_manager.h>

static crude_entity
crude_node_manager_load_node_from_file_
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *absolute_filepath
);

static crude_entity
crude_node_manager_load_node_from_json_
(
  _In_ crude_node_manager                                 *manager,
  _In_ cJSON                                              *node_json
);

static cJSON*
crude_node_manager_parse_json_
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *absolute_filepath
);

static cJSON*
crude_node_manager_node_to_json_hierarchy_
(
  _In_ crude_node_manager                                 *manager,
  _In_ crude_entity                                        node
);

void
crude_node_destroy_hierarchy_
(
  _In_ crude_entity                                        entity
);

void
crude_node_manager_initialize
(
  _In_ crude_node_manager                                 *manager,
  _In_ crude_node_manager_creation const                  *creation
)
{
  manager->world = creation->world;
  manager->temporary_allocator = creation->temporary_allocator;
  manager->additional_parse_json_to_component_func = creation->additional_parse_json_to_component_func;
  manager->additional_parse_all_components_to_json_func = creation->additional_parse_all_components_to_json_func;
  manager->resources_absolute_directory = creation->resources_absolute_directory;
  manager->physics_resources_manager = creation->physics_resources_manager;
  manager->collisions_resources_manager = creation->collisions_resources_manager;
  manager->allocator = creation->allocator;

  crude_linear_allocator_initialize( &manager->string_linear_allocator, CRUDE_SCENE_STRING_LINEAR_ALLOCATOR_SIZE, "scene_allocator" );
  crude_string_buffer_initialize( &manager->string_bufffer, 1024, crude_linear_allocator_pack( &manager->string_linear_allocator ) );
  CRUDE_HASHMAP_INITIALIZE( manager->hashed_absolute_filepath_to_node, crude_heap_allocator_pack( manager->allocator ) );
}

void
crude_node_manager_deinitialize
(
  _In_ crude_node_manager                                 *manager
)
{
  CRUDE_HASHMAP_DEINITIALIZE( manager->hashed_absolute_filepath_to_node );
  crude_string_buffer_deinitialize( &manager->string_bufffer );
  crude_linear_allocator_deinitialize( &manager->string_linear_allocator );
}

void
crude_node_manager_clear
(
  _In_ crude_node_manager                                 *manager
)
{
  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( manager->hashed_absolute_filepath_to_node ); ++i )
  {
    if ( crude_hashmap_backet_key_valid( manager->hashed_absolute_filepath_to_node[ i ].key ) )
    {
      crude_node_destroy_hierarchy_( manager->hashed_absolute_filepath_to_node[ i ].value );
    }
    manager->hashed_absolute_filepath_to_node[ i ].key = 0;
  }

  crude_string_buffer_clear( &manager->string_bufffer );
  crude_linear_allocator_clear( &manager->string_linear_allocator );
}

crude_entity
crude_node_manager_get_node
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_absolute_filepath
)
{
  crude_entity                                             node_template;
  int64                                                    node_index;
  uint64                                                   filename_hashed;

  filename_hashed = crude_hash_string( node_absolute_filepath, 0 );
  node_index = CRUDE_HASHMAP_GET_INDEX( manager->hashed_absolute_filepath_to_node, filename_hashed );
  if ( node_index == -1 )
  {
    node_template = crude_node_manager_load_node_from_file_( manager, node_absolute_filepath );
    CRUDE_HASHMAP_SET( manager->hashed_absolute_filepath_to_node, filename_hashed, node_template );
  }
  else
  {
    node_template = manager->hashed_absolute_filepath_to_node[ node_index ].value;
  }

  return node_template;
}

void
crude_node_manager_remove_node
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_absolute_filepath
)
{
  int64                                                    node_index;
  uint64                                                   filename_hashed;

  filename_hashed = crude_hash_string( node_absolute_filepath, 0 );
  node_index = CRUDE_HASHMAP_GET_INDEX( manager->hashed_absolute_filepath_to_node, filename_hashed );
  if ( node_index == -1 )
  {
    return;
  }
  
  crude_node_destroy_hierarchy_( manager->hashed_absolute_filepath_to_node[ node_index ].value );
  CRUDE_HASHMAP_REMOVE( manager->hashed_absolute_filepath_to_node, filename_hashed );
}

void
crude_node_manager_save_node_by_file_to_file
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_absolute_filepath,
  _In_ char const                                         *saved_absolute_filepath
)
{
  int64                                                    node_index;
  uint64                                                   filename_hashed;

  filename_hashed = crude_hash_string( node_absolute_filepath, 0 );
  node_index = CRUDE_HASHMAP_GET_INDEX( manager->hashed_absolute_filepath_to_node, filename_hashed );
  if ( node_index != -1 )
  {
    return;
  }
  
  crude_node_manager_save_node_to_file( manager, manager->hashed_absolute_filepath_to_node[ node_index ].value, saved_absolute_filepath );
}

void
crude_node_manager_save_node_to_file
(
  _In_ crude_node_manager                                 *manager,
  _In_ crude_entity                                        node,
  _In_ char const                                         *saved_absolute_filepath
)
{
  cJSON                                                   *node_json;
  char const                                              *node_str;
  node_json = crude_node_manager_node_to_json_hierarchy_( manager, node );
  node_str = cJSON_Print( node_json );
  crude_write_file( saved_absolute_filepath, node_str, strlen( node_str ) ); // TODO
  cJSON_Delete( node_json );
}

crude_entity
crude_node_manager_load_node_from_file_
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *absolute_filepath
)
{
  cJSON                                                   *node_json;
  crude_entity                                             node;
  size_t                                                   temporary_allocated_marker;
  
  
  temporary_allocated_marker = crude_stack_allocator_get_marker( manager->temporary_allocator );
 
  node_json = crude_node_manager_parse_json_( manager, absolute_filepath );
  if ( node_json == NULL )
  {
    return CRUDE_COMPOUNT_EMPTY( crude_entity );
  }
  node = crude_node_manager_load_node_from_json_( manager, node_json );

  cJSON_Delete( node_json );
  crude_stack_allocator_free_marker( manager->temporary_allocator, temporary_allocated_marker );

  return node;
}

crude_entity
crude_node_manager_load_node_from_json_
(
  _In_ crude_node_manager                                 *manager,
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
    
    temporary_allocator_marker = crude_stack_allocator_get_marker( manager->temporary_allocator );
    crude_string_buffer_initialize( &temporary_path_buffer, 2048, crude_stack_allocator_pack( manager->temporary_allocator ) );

    node_external_json_filepath = cJSON_GetStringValue(  cJSON_GetObjectItemCaseSensitive( node_json, "external") );
    
    node_external_json = crude_node_manager_parse_json_( manager, crude_string_buffer_append_use_f( &temporary_path_buffer, "%s%s", manager->resources_absolute_directory, node_external_json_filepath ) );
    node = crude_node_manager_load_node_from_json_( manager, node_external_json );
    crude_entity_set_name( node, node_name );

    CRUDE_ENTITY_SET_COMPONENT( node, crude_node_external, { crude_string_buffer_append_use_f( &manager->string_bufffer, "%s", node_external_json_filepath ) } );

    crude_stack_allocator_free_marker( manager->temporary_allocator, temporary_allocator_marker );
  }
  else
  {
    node = crude_entity_create_empty( manager->world, node_name );
  }
  
  if ( !is_node_external )
  {
    cJSON                                                 *children_json;
    cJSON                                                 *child_json;
  
    children_json = cJSON_GetObjectItemCaseSensitive( node_json, "children" );
    for ( uint32 child_index = 0; child_index < cJSON_GetArraySize( children_json ); ++child_index )
    {
      child_json = cJSON_GetArrayItem( children_json, child_index );
      crude_entity child_node = crude_node_manager_load_node_from_json_( manager, child_json );
      crude_entity_set_parent( child_node, node );
    }
  }

  {
    cJSON const                                           *node_transform_json;
    cJSON const                                           *node_components_json;
    cJSON const                                           *node_tags_json;
    crude_transform                                        transform;
  
    node_transform_json = cJSON_GetObjectItemCaseSensitive( node_json, "transform" );
    node_components_json = cJSON_GetObjectItemCaseSensitive( node_json, "components" );
    node_tags_json = cJSON_GetObjectItemCaseSensitive( node_json, "tags" );

    CRUDE_PARSE_JSON_TO_COMPONENT( crude_transform )( &transform, node_transform_json, node, manager );
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
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_gltf )( &gltf, component_json, node, manager );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_gltf, { gltf } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_camera ) ) == 0 )
      {
        crude_camera                                       camera;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_camera )( &camera, component_json, node, manager );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_camera, { camera } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_free_camera ) ) == 0 )
      {
        crude_free_camera                                  free_camera;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_free_camera )( &free_camera, component_json, node, manager );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_free_camera, { free_camera } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_light ) ) == 0 )
      {
        crude_light                                        light;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_light )( &light, component_json, node, manager );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_light, { light } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_physics_static_body_handle ) ) == 0 )
      {
        crude_physics_static_body_handle                   static_body;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_physics_static_body_handle )( &static_body, component_json, node, manager );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_static_body_handle, { static_body } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_physics_character_body_handle ) ) == 0 )
      {
        crude_physics_character_body_handle                  dynamic_body;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_physics_character_body_handle )( &dynamic_body, component_json, node, manager );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_character_body_handle, { dynamic_body } );
      }
      else if ( crude_string_cmp( component_type, CRUDE_COMPONENT_STRING( crude_physics_collision_shape ) ) == 0 )
      {
        crude_physics_collision_shape                      collision_shape;
        CRUDE_PARSE_JSON_TO_COMPONENT( crude_physics_collision_shape )( &collision_shape, component_json, node, manager );
        CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_collision_shape, { collision_shape } );
      }
      else
      {
        manager->additional_parse_json_to_component_func( node, component_json, component_type, manager );
      }
    }
  }

  return node;
}

cJSON*
crude_node_manager_parse_json_
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *absolute_filepath
)
{
  cJSON                                                 *json;
  uint8                                                 *json_buffer;
  uint32                                                 json_buffer_size;
  
  if ( !crude_file_exist( absolute_filepath ) )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse scene", absolute_filepath );
    return NULL;
  }
  
  crude_read_file( absolute_filepath, crude_stack_allocator_pack( manager->temporary_allocator ), &json_buffer, &json_buffer_size );
  
  json = cJSON_ParseWithLength( CRUDE_REINTERPRET_CAST( char const*, json_buffer ), json_buffer_size );
  if ( !json )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file for scene... Error %s", cJSON_GetErrorPtr() );
    return NULL;
  }
}

cJSON*
crude_node_manager_node_to_json_hierarchy_
(
  _In_ crude_node_manager                                 *manager,
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
    crude_physics_character_body_handle const             *dynamic_body;
    crude_physics_collision_shape const                   *collision_shape;

    node_components_json = cJSON_AddArrayToObject( node_json, "components" );
    
    node_camera = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_camera );
    if ( node_camera )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_camera )( node_camera, manager ) );
    }
    
    node_gltf = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_gltf );
    if ( node_gltf )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_gltf )( node_gltf, manager ) );
      is_gltf_node = true;
    }
    
    node_free_camera = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_free_camera );
    if ( node_free_camera )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_free_camera )( node_free_camera, manager ) );
    }

    node_light = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_light );
    if ( node_light )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_light )( node_light, manager ) );
    }
    
    static_body = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_physics_static_body_handle );
    if ( static_body )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_physics_static_body_handle )( static_body, manager ) );
    }
    
    dynamic_body = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_physics_character_body_handle );
    if ( dynamic_body )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_physics_character_body_handle )( dynamic_body, manager ) );
    }
    
    collision_shape = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_physics_collision_shape );
    if ( collision_shape )
    {
      cJSON_AddItemToArray( node_components_json, CRUDE_PARSE_COMPONENT_TO_JSON( crude_physics_collision_shape )( collision_shape, manager ) );
    }
    
    manager->additional_parse_all_components_to_json_func( node, node_components_json, manager );
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
          if ( !CRUDE_ENTITY_HAS_COMPONENT( child, crude_node_runtime ) )
          {
            cJSON_AddItemToArray( children_json, crude_node_manager_node_to_json_hierarchy_( manager, child ) );
          }
        }
      }
    }
  }

  return node_json;
}

void
crude_node_destroy_hierarchy_
(
  _In_ crude_entity                                        node
)
{
  if ( !crude_entity_valid( node ) )
  {
    return;
  }

  ecs_iter_t it = ecs_children( node.world, node.handle );
  while ( ecs_children_next( &it ) )
  {
    for ( size_t i = 0; i < it.count; ++i )
    {
      crude_entity child = CRUDE_COMPOUNT( crude_entity, { .handle = it.entities[ i ], .world = node.world } );
      crude_entity_destroy_hierarchy( child );
    }
  }

  crude_entity_destroy( node );
}