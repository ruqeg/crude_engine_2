#include <thirdparty/cJSON/cJSON.h>

#include <engine/core/assert.h>
#include <engine/core/file.h>
#include <engine/core/string.h>
#include <engine/core/array.h>
#include <engine/scene/scene_ecs.h>
#include <engine/physics/physics_ecs.h>
#include <engine/scene/scripts/free_camera_ecs.h>

#include <engine/scene/node_manager.h>

static crude_entity
crude_node_manager_load_node_from_file_
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *absolute_filepath,
  _In_ crude_ecs                                          *world
);

static crude_entity
crude_node_manager_load_node_from_json_
(
  _In_ crude_node_manager                                 *manager,
  _In_ cJSON                                              *node_json,
  _In_ crude_ecs                                          *world,
  _In_opt_ crude_entity                                   *parent
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
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
);

void
crude_node_destroy_hierarchy_
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        entity
);

void
crude_node_manager_initialize
(
  _In_ crude_node_manager                                 *manager,
  _In_ crude_node_manager_creation const                  *creation
)
{
  manager->temporary_allocator = creation->temporary_allocator;
  manager->components_serialization_manager = creation->components_serialization_manager;
  manager->resources_absolute_directory = creation->resources_absolute_directory;
  manager->physics_resources_manager = creation->physics_resources_manager;
  manager->collisions_resources_manager = creation->collisions_resources_manager;
  manager->model_renderer_resources_manager = creation->model_renderer_resources_manager;
  manager->allocator = creation->allocator;
  manager->select_camera_func = creation->select_camera_func;
  manager->select_camera_ctx = creation->select_camera_ctx;

  CRUDE_HASHMAPSTR_INITIALIZE( manager->relative_filepath_to_node, crude_heap_allocator_pack( manager->allocator ) );
  crude_string_buffer_initialize( &manager->absolute_filepath_string_buffer, CRUDE_RMEGA( 1 ), crude_heap_allocator_pack( manager->allocator ) );
  crude_string_buffer_initialize( &manager->relative_filepath_string_buffer, CRUDE_NODE_COUNT_MAX * ( 1 + CRUDE_NODE_RELATIVE_FILEPATH_LENGTH_MAX ), crude_heap_allocator_pack( manager->allocator ) );
}

void
crude_node_manager_deinitialize
(
  _In_ crude_node_manager                                 *manager
)
{
  crude_string_buffer_deinitialize( &manager->absolute_filepath_string_buffer );
  crude_string_buffer_deinitialize( &manager->relative_filepath_string_buffer );
  CRUDE_HASHMAPSTR_DEINITIALIZE( manager->relative_filepath_to_node );
}

void
crude_node_manager_clear
(
  _In_ crude_node_manager                                 *manager,
  _In_ crude_ecs                                          *world
)
{
  for ( uint32 i = 0; i < CRUDE_HASHMAPSTR_CAPACITY( manager->relative_filepath_to_node ); ++i )
  {
    if ( crude_hashmapstr_backet_key_hash_valid( manager->relative_filepath_to_node[ i ].key.key_hash ) )
    {
      crude_node_destroy_hierarchy_( world, manager->relative_filepath_to_node[ i ].value );
    }
    manager->relative_filepath_to_node[ i ].key.key_hash = CRUDE_HASHMAPSTR_BACKET_STATE_EMPTY;
  }

  crude_string_buffer_clear( &manager->relative_filepath_string_buffer );
}

crude_entity
crude_node_manager_get_node
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_realtive_filepath,
  _In_ crude_ecs                                          *world
)
{
  char const                                              *node_absolute_filepath;
  crude_entity                                             node_template;
  int64                                                    node_index;

  crude_string_buffer_clear( &manager->absolute_filepath_string_buffer );
  node_absolute_filepath = crude_string_buffer_append_use_f( &manager->absolute_filepath_string_buffer, "%s%s", manager->resources_absolute_directory, node_realtive_filepath );

  node_index = CRUDE_HASHMAPSTR_GET_INDEX( manager->relative_filepath_to_node, node_realtive_filepath );
  if ( node_index == -1 )
  {
    node_template = crude_node_manager_load_node_from_file_( manager, node_absolute_filepath, world );
    
    CRUDE_HASHMAPSTR_SET(
      manager->relative_filepath_to_node,
      CRUDE_COMPOUNT( crude_string_link, { crude_string_buffer_append_use_f( &manager->relative_filepath_string_buffer, "%s", node_realtive_filepath ) } ),
      node_template );
  }
  else
  {
    node_template = manager->relative_filepath_to_node[ node_index ].value;
  }

  return node_template;
}

void
crude_node_manager_remove_node
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_realtive_filepath,
  _In_ crude_ecs                                          *world
)
{
  char const                                              *node_absolute_filepath;
  int64                                                    node_index;
  
  crude_string_buffer_clear( &manager->absolute_filepath_string_buffer );
  node_absolute_filepath = crude_string_buffer_append_use_f( &manager->absolute_filepath_string_buffer, "%s%s", manager->resources_absolute_directory, node_realtive_filepath );

  node_index = CRUDE_HASHMAPSTR_GET_INDEX( manager->relative_filepath_to_node, node_realtive_filepath );
  if ( node_index == -1 )
  {
    return;
  }
  
  crude_node_destroy_hierarchy_( world, manager->relative_filepath_to_node[ node_index ].value );
  CRUDE_HASHMAPSTR_REMOVE( manager->relative_filepath_to_node, node_realtive_filepath );
}

void
crude_node_manager_save_node_by_file_to_file
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *node_realtive_filepath,
  _In_ char const                                         *saved_relative_filepath,
  _In_ crude_ecs                                          *world
)
{
  char const                                              *saved_absolute_filepath;
  int64                                                    node_index;
    
  node_index = CRUDE_HASHMAPSTR_GET_INDEX( manager->relative_filepath_to_node, node_realtive_filepath );
  if ( node_index != -1 )
  {
    return;
  }

  crude_string_buffer_clear( &manager->absolute_filepath_string_buffer );
  saved_absolute_filepath = crude_string_buffer_append_use_f( &manager->absolute_filepath_string_buffer, "%s%s", manager->resources_absolute_directory, saved_relative_filepath );

  crude_node_manager_save_node_to_file( manager, world, manager->relative_filepath_to_node[ node_index ].value, saved_absolute_filepath );
}

void
crude_node_manager_save_node_to_file
(
  _In_ crude_node_manager                                 *manager,
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node,
  _In_ char const                                         *saved_relative_filepath
)
{
  char const                                              *saved_absolute_filepath;
  cJSON                                                   *node_json;
  char const                                              *node_str;
  
  crude_string_buffer_clear( &manager->absolute_filepath_string_buffer );
  saved_absolute_filepath = crude_string_buffer_append_use_f( &manager->absolute_filepath_string_buffer, "%s%s", manager->resources_absolute_directory, saved_relative_filepath );

  node_json = crude_node_manager_node_to_json_hierarchy_( manager, world, node );
  node_str = cJSON_Print( node_json );
  crude_write_file( saved_absolute_filepath, node_str, crude_string_length( node_str ) ); // TODO
  cJSON_Delete( node_json );
}

crude_entity
crude_node_manager_load_node_from_file_
(
  _In_ crude_node_manager                                 *manager,
  _In_ char const                                         *absolute_filepath,
  _In_ crude_ecs                                          *world
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

  node = crude_node_manager_load_node_from_json_( manager, node_json, world, NULL );

  cJSON_Delete( node_json );
  crude_stack_allocator_free_marker( manager->temporary_allocator, temporary_allocated_marker );

  return node;
}

crude_entity
crude_node_manager_load_node_from_json_
(
  _In_ crude_node_manager                                 *manager,
  _In_ cJSON                                              *node_json,
  _In_ crude_ecs                                          *world,
  _In_opt_ crude_entity                                   *parent
)
{
  crude_entity                                             node;
  char const                                              *node_name;
  bool                                                     is_node_external;
  
  node_name = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( node_json, "name") );

  is_node_external = cJSON_HasObjectItem( node_json, "external" );
  if ( is_node_external )
  {
    char const                                            *node_external_relative_filepath;
    crude_node_external                                    node_external_component;

    CRUDE_ASSERT( parent ); /* WTF IS GOING ONE, DONT PUT EXTERNAL ON TOP OF SCENE */

    node_external_relative_filepath = cJSON_GetStringValue(  cJSON_GetObjectItemCaseSensitive( node_json, "external") );
    
    node = crude_node_copy_hierarchy( world, crude_node_manager_get_node( manager, node_external_relative_filepath, world ), node_name, *parent, true, true );
    
    node_external_component = crude_node_external_empty( );
    crude_string_copy( node_external_component.node_relative_filepath, node_external_relative_filepath, sizeof( node_external_component ) );
    CRUDE_ENTITY_SET_COMPONENT( world, node, crude_node_external, { node_external_component } );
  }
  else
  {
    node = crude_entity_create_empty( world, node_name );
    
    if ( parent )
    {
      crude_entity_set_parent( world, node, *parent );
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
      crude_entity child_node = crude_node_manager_load_node_from_json_( manager, child_json, world, &node );
    }
  }

  {
    cJSON const                                           *node_components_json;
  
    node_components_json = cJSON_GetObjectItemCaseSensitive( node_json, "components" );

    for ( uint32 component_index = 0; component_index < cJSON_GetArraySize( node_components_json ); ++component_index )
    {
      cJSON const                                       *component_json;
      cJSON const                                       *component_type_json;
      char const                                        *component_type;

      component_json = cJSON_GetArrayItem( node_components_json, component_index );
      component_type_json = cJSON_GetObjectItemCaseSensitive( component_json, "type" );
      component_type = cJSON_GetStringValue( component_type_json );
  
      int64 index = CRUDE_HASHMAPSTR_GET_INDEX( manager->components_serialization_manager->component_name_to_json_funs, component_type );
      CRUDE_ASSERT( index != -1 );
      manager->components_serialization_manager->component_name_to_json_funs[ index ].value( world, node, component_json, manager );
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
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
)
{
  cJSON                                                   *node_json;
  bool                                                     is_external_node, is_gltf_node;
  
  is_external_node = is_gltf_node = false;

  node_json = cJSON_CreateObject( );
  
  cJSON_AddItemToObject( node_json, "name", cJSON_CreateString( crude_entity_get_name( world, node ) ) );

  is_external_node = CRUDE_ENTITY_HAS_COMPONENT( world, node, crude_node_external );
  if ( is_external_node )
  {
    cJSON_AddItemToObject( node_json, "external", cJSON_CreateString( CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, node, crude_node_external )->node_relative_filepath ) );
  }
  
  {
    cJSON                                                 *node_components_json;

    node_components_json = cJSON_AddArrayToObject( node_json, "components" );
    
    for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( manager->components_serialization_manager->component_id_to_json_funs ); ++i )
    {
      if ( crude_hashmap_backet_key_valid( manager->components_serialization_manager->component_id_to_json_funs[ i ].key ) )
      {
        cJSON* component_json = manager->components_serialization_manager->component_id_to_json_funs[ i ].value( world, node, manager );
        if ( component_json )
        {
          cJSON_AddItemToArray( node_components_json, component_json );
        }
      }
    }
  }
  
  {
    cJSON                                                 *children_json;

    children_json = cJSON_AddArrayToObject( node_json, "children" );
    
    if ( !is_gltf_node && !is_external_node )
    {
      ecs_iter_t it = crude_ecs_children( world, node );
      while ( ecs_children_next( &it ) )
      {
        for ( size_t i = 0; i < it.count; ++i )
        {
          crude_entity                                       child;

          child = crude_entity_from_iterator( &it, i );
          cJSON_AddItemToArray( children_json, crude_node_manager_node_to_json_hierarchy_( manager, world, child ) );
        }
      }
    }
  }

  return node_json;
}

void
crude_node_destroy_hierarchy_
(
  _In_ crude_ecs                                          *world,
  _In_ crude_entity                                        node
)
{
  if ( !crude_entity_valid( world, node ) )
  {
    return;
  }

  ecs_iter_t it = crude_ecs_children( world, node );
  while ( ecs_children_next( &it ) )
  {
    for ( size_t i = 0; i < it.count; ++i )
    {
      crude_entity child = crude_entity_from_iterator( &it, i );
      crude_entity_destroy_hierarchy( world, child );
    }
  }

  crude_entity_destroy( world, node );
}