#include <cJSON.h>

#include <engine/core/file.h>
#include <engine/core/log.h>

#include <engine/engine/environment.h>

void
crude_environment_initialize
(
  _In_ crude_environment                                  *environment,
  _In_ char const                                          *absolute_filepath,
  _In_ char const                                         *working_absolute_directory,
  _In_ crude_heap_allocator                                *heap_allocator,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  cJSON                                                   *json;
  uint8                                                   *json_buffer;
  size_t                                                   allocated_marker;
  uint32                                                   json_buffer_size;

  allocated_marker = crude_stack_allocator_get_marker( temporary_allocator );
  crude_read_file( absolute_filepath, crude_stack_allocator_pack( temporary_allocator ), &json_buffer, &json_buffer_size );

  json = cJSON_ParseWithLength( CRUDE_REINTERPRET_CAST( char const*, json_buffer ), json_buffer_size );
  if ( !json )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file for game... Error %s", cJSON_GetErrorPtr() );
    goto cleanup;
  }
  
  {
    cJSON                                                 *directories_json;
    uint64                                                 constant_string_buffer_size, working_absolute_directory_length;
    
    constant_string_buffer_size = 0u;
    working_absolute_directory_length = crude_string_length( working_absolute_directory ) + 1;

    directories_json = cJSON_GetObjectItemCaseSensitive( json, "directories" );
    char const *render_graph_relative_directory = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( directories_json, "render_graph_relative_directory" ) );
    char const *resources_relative_directory = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( directories_json, "resources_relative_directory" ) );
    char const *techniques_relative_directory = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( directories_json, "techniques_relative_directory" ) );
    char const *shaders_relative_directory = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( directories_json, "shaders_relative_directory" ) );
    char const *compiled_shaders_relative_directory = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( directories_json, "compiled_shaders_relative_directory" ) );

    constant_string_buffer_size += working_absolute_directory_length + crude_string_length( render_graph_relative_directory );
    constant_string_buffer_size += working_absolute_directory_length + crude_string_length( resources_relative_directory );
    constant_string_buffer_size += working_absolute_directory_length + crude_string_length( techniques_relative_directory );
    constant_string_buffer_size += working_absolute_directory_length + crude_string_length( shaders_relative_directory );
    constant_string_buffer_size += working_absolute_directory_length + crude_string_length( compiled_shaders_relative_directory );

    crude_string_buffer_initialize( &environment->constant_string_buffer, constant_string_buffer_size, crude_heap_allocator_pack( heap_allocator ) );
    environment->directories.render_graph_absolute_directory = crude_string_buffer_append_use_f( &environment->constant_string_buffer, "%s%s", working_absolute_directory, render_graph_relative_directory );
    environment->directories.resources_absolute_directory = crude_string_buffer_append_use_f( &environment->constant_string_buffer, "%s%s", working_absolute_directory, resources_relative_directory );
    environment->directories.techniques_absolute_directory = crude_string_buffer_append_use_f( &environment->constant_string_buffer, "%s%s", working_absolute_directory, techniques_relative_directory );
    environment->directories.shaders_absolute_directory = crude_string_buffer_append_use_f( &environment->constant_string_buffer, "%s%s", working_absolute_directory, shaders_relative_directory );
    environment->directories.compiled_shaders_absolute_directory = crude_string_buffer_append_use_f( &environment->constant_string_buffer, "%s%s", working_absolute_directory, compiled_shaders_relative_directory );
  }
  
  {
    cJSON                                                 *window_json;
    
    window_json = cJSON_GetObjectItemCaseSensitive( json, "window" );
    crude_snprintf( environment->window.initial_title, CRUDE_COUNTOF( environment->window.initial_title ), cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( window_json, "title" ) ) );
    environment->window.initial_width = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( window_json, "width" ) );
    environment->window.initial_height = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( window_json, "height" ) );
  }

  cJSON_Delete( json );
cleanup:
  crude_stack_allocator_free_marker( temporary_allocator, allocated_marker );
}

void
crude_environment_deinitialize
(
  _In_ crude_environment                                  *environment
)
{
  crude_string_buffer_deinitialize( &environment->constant_string_buffer );
}