#include <cJSON.h>

#include <core/assert.h>
#include <core/file.h>
#include <core/string.h>
#include <core/array.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <physics/physics_components.h>

#include <scene/scene.h>

static crude_physics_box_collision_shape
json_object_to_crude_physics_box_collision_shape_
(
  _In_ cJSON const                                        *json
)
{
  crude_physics_box_collision_shape shape = CRUDE_COMPOUNT_EMPTY( crude_physics_box_collision_shape );
  CRUDE_PARSE_JSON_TO_COMPONENT( XMFLOAT3 )( &shape.half_extent, cJSON_GetObjectItem( json, "half_extent" ) );
  return shape;
}

static crude_physics_sphere_collision_shape
json_object_to_crude_physics_sphere_collision_shape_
(
  _In_ cJSON const                                        *json
)
{
  crude_physics_sphere_collision_shape shape = CRUDE_COMPOUNT_EMPTY( crude_physics_sphere_collision_shape );
  shape.radius = cJSON_GetNumberValue( cJSON_GetObjectItem( json, "radius" ) );
  return shape;
}

static cJSON*
crude_physics_box_collision_shape_to_json_object_
(
  _In_ crude_physics_box_collision_shape const            *shape
)
{
  cJSON *shape_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( shape_json, "half_extent", cJSON_CreateFloatArray( &shape->half_extent.x, 3  ) );
  return shape_json;
}

static cJSON*
crude_physics_sphere_collision_shape_to_json_object_
(
  _In_ crude_physics_sphere_collision_shape const         *shape
)
{
  cJSON *shape_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( shape_json, "radius", cJSON_CreateNumber( shape->radius ) );
  return shape_json;
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
    crude_transform                                        transform;
  
    node_json = hierarchy_json;//cJSON_GetObjectItemCaseSensitive( hierarchy_json, "node" ); 
    node_name = cJSON_GetStringValue(  cJSON_GetObjectItemCaseSensitive( node_json, "name") );
    node_transform_json = cJSON_GetObjectItemCaseSensitive( node_json, "transform" );
    node_components_json = cJSON_GetObjectItemCaseSensitive( node_json, "components" );
    node_tags_json = cJSON_GetObjectItemCaseSensitive( node_json, "tags" );
  
    node = crude_entity_create_empty( scene->world, node_name );

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
      else if ( crude_string_cmp( component_type, "crude_physics_static_body" ) == 0 )
      {
        if ( cJSON_HasObjectItem( component_json, "crude_physics_box_collision_shape" ) )
        {
          CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_static_body, {
            .box_shape = json_object_to_crude_physics_box_collision_shape_( cJSON_GetObjectItemCaseSensitive( component_json, "crude_physics_box_collision_shape" ) ),
            .collision_shape_type = CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX
          } );
        }
        else if ( cJSON_HasObjectItem( component_json, "crude_physics_sphere_collision_shape" ) )
        {
          CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_static_body, {
            .sphere_shape = json_object_to_crude_physics_sphere_collision_shape_( cJSON_GetObjectItemCaseSensitive( component_json, "crude_physics_sphere_collision_shape" ) ),
            .collision_shape_type = CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE
          } );
        }
        else
        {
          CRUDE_ASSERT( false );
        }
      }
      else if ( crude_string_cmp( component_type, "crude_physics_dynamic_body" ) == 0 )
      {
        if ( cJSON_HasObjectItem( component_json, "crude_physics_box_collision_shape" ) )
        {
          CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_dynamic_body, {
            .box_shape = json_object_to_crude_physics_box_collision_shape_( cJSON_GetObjectItemCaseSensitive( component_json, "crude_physics_box_collision_shape" ) ),
            .collision_shape_type = CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX
          } );
        }
        else if ( cJSON_HasObjectItem( component_json, "crude_physics_sphere_collision_shape" ) )
        {
          CRUDE_ENTITY_SET_COMPONENT( node, crude_physics_dynamic_body, {
            .sphere_shape = json_object_to_crude_physics_sphere_collision_shape_( cJSON_GetObjectItemCaseSensitive( component_json, "crude_physics_sphere_collision_shape" ) ),
            .collision_shape_type = CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE
          } );
        }
        else
        {
          CRUDE_ASSERT( false );
        }
      }
      else
      {
        scene->additional_parse_json_to_component_func( node, component_json, component_type );
      }
    }
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
  scene->additional_parse_json_to_component_func = creation->additional_parse_json_to_component_func;
  scene->additional_parse_all_components_to_json_func = creation->additional_parse_all_components_to_json_func;

  crude_string_buffer_initialize( &scene->path_bufffer, 512, scene->allocator_container );

  {
    char working_directory[ 512 ];
    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    scene->resources_path = crude_string_buffer_append_use_f( &scene->path_bufffer, "%s%s", working_directory, creation->resources_path );
  }

  allocated_marker = crude_stack_allocator_get_marker( scene->temporary_allocator );
  
  crude_string_buffer_initialize( &temporary_path_buffer, 1024, crude_stack_allocator_pack( scene->temporary_allocator ) );
  
  /* Parse json */
  {
    uint8                                                 *json_buffer;
    uint32                                                 json_buffer_size;

    if ( !crude_file_exist( creation->filepath ) )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse render graph", creation->filepath );
      return;
    }

    crude_read_file( creation->filepath, crude_stack_allocator_pack( scene->temporary_allocator ), &json_buffer, &json_buffer_size );

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

  node_json = cJSON_CreateObject( );
  
  cJSON_AddItemToObject( node_json, "name", cJSON_CreateString( crude_entity_get_name( node ) ) );

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
    crude_physics_static_body const                       *static_body;
    crude_physics_dynamic_body const                      *dynamic_body;

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

    static_body = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_physics_static_body );
    if ( static_body )
    {
      cJSON                                               *node_physics_static_body_json;

      node_physics_static_body_json = cJSON_CreateObject( );
       
      cJSON_AddItemToObject( node_physics_static_body_json, "type", cJSON_CreateString( "crude_physics_static_body" ) );

      if ( static_body->collision_shape_type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
      {
        cJSON_AddItemToObject( node_physics_static_body_json, "crude_physics_box_collision_shape", crude_physics_box_collision_shape_to_json_object_( &static_body->box_shape  ) );
      }
      else if ( static_body->collision_shape_type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
      {
        cJSON_AddItemToObject( node_physics_static_body_json, "crude_physics_sphere_collision_shape", crude_physics_sphere_collision_shape_to_json_object_( &static_body->sphere_shape  ) );
      }
      else
      {
        CRUDE_ASSERT( false );
      }
      
      cJSON_AddItemToArray( node_components_json, node_physics_static_body_json );
    }

    dynamic_body = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( node, crude_physics_dynamic_body );
    if ( dynamic_body )
    {
      cJSON                                               *node_physics_dynamic_body_json;

      node_physics_dynamic_body_json = cJSON_CreateObject( );
       
      cJSON_AddItemToObject( node_physics_dynamic_body_json, "type", cJSON_CreateString( "crude_physics_dynamic_body" ) );

      if ( dynamic_body->collision_shape_type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
      {
        cJSON_AddItemToObject( node_physics_dynamic_body_json, "crude_physics_box_collision_shape", crude_physics_box_collision_shape_to_json_object_( &dynamic_body->box_shape  ) );
      }
      else if ( dynamic_body->collision_shape_type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
      {
        cJSON_AddItemToObject( node_physics_dynamic_body_json, "crude_physics_sphere_collision_shape", crude_physics_sphere_collision_shape_to_json_object_( &dynamic_body->sphere_shape  ) );
      }
      else
      {
        CRUDE_ASSERT( false );
      }
      
      cJSON_AddItemToArray( node_components_json, node_physics_dynamic_body_json );
    }

    scene->additional_parse_all_components_to_json_func( node, node_components_json );
  }
  
  {
    cJSON                                                 *children_json;

    children_json = cJSON_AddArrayToObject( node_json, "children" );
    
    if ( !CRUDE_ENTITY_HAS_COMPONENT( node, crude_gltf ) )
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

  scene_json = cJSON_CreateObject( );
  cJSON_AddItemToObject( scene_json, "hierarchy", node_to_json_hierarchy_( scene, scene->main_node ) );
  
  scene_str = cJSON_Print( scene_json );
  crude_write_file( filename, scene_str, strlen( scene_str ) ); // TODO
  
  cJSON_Delete( scene_json );
}