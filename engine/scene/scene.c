#include <cJSON.h>

#include <core/assert.h>
#include <core/file.h>
#include <core/string.h>

#include <scene/scene.h>

void
crude_scene_initialize
(
  _In_ crude_scene                                        *scene,
  _In_ crude_scene_creation const                         *creation
)
{
  scene->allocator_container = creation->allocator_container;
  scene->temporary_allocator = creation->temporary_allocator;
  
  crude_string_buffer_initialize( &scene->path_bufffer, 512, scene->allocator_container );

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
  char const                                              *json_path;
  crude_string_buffer                                      temporary_path_buffer;

  allocated_marker = crude_stack_allocator_get_marker( scene->temporary_allocator );
  
  crude_string_buffer_initialize( &temporary_path_buffer, 1024, crude_stack_allocator_pack( scene->temporary_allocator ) );

  json_path = crude_string_buffer_append_use_f( &temporary_path_buffer, "%s%s", scene->resources_path, json_name );
  if ( !crude_file_exist( json_path ) )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse render graph", json_path );
    return;
  }
  
  /* Parse json */
  {
    uint8                                                 *json_buffer;
    size_t                                                 json_buffer_size;
    crude_read_file( json_path, crude_stack_allocator_pack( scene->temporary_allocator ), &json_buffer, &json_buffer_size );

    scene_json = cJSON_ParseWithLength( json_buffer, json_buffer_size );
    if ( !scene_json )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file for scene... Error %s", cJSON_GetErrorPtr() );
      return;
    }
  }

  cJSON_Delete( scene_json );
  crude_stack_allocator_free_marker( scene->temporary_allocator, allocated_marker  );
}