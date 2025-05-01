#include <cJSON.h>

#include <core/file.h>
#include <core/log.h>
#include <core/assert.h>
#include <core/string.h>

#include <graphics/render_graph.h>

static crude_gfx_render_graph_resource_type
_string_to_resource_type
(
  _In_ char const                                         *input_type
)
{
  if ( strcmp( input_type, "texture" ) == 0 )
  {
    return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE;
  }
  
  if ( strcmp( input_type, "attachment" ) == 0 )
  {
    return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT;
  }
  
  if ( strcmp( input_type, "buffer" ) == 0 )
  {
    return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_BUFFER;
  }
  
  if ( strcmp( input_type, "reference" ) == 0 )
  {
    return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_REFERENCE;
  }
  
  CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "Can't convert string to resoruce type for render graph" );
  return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_INVALID;
}

static crude_gfx_render_pass_operation
string_to_render_pass_operation
(
  _In_ char const                                         *op
)
{
  if ( strcmp( op, "clear" ) == 0 )
  {
    return CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;
  }
  else if ( strcmp( op, "load" ) == 0 )
  {
    return CRUDE_GFX_RENDER_PASS_OPERATION_LOAD;
  }
  
  CRUDE_ASSERT( false );
  return CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
}

void
crude_gfx_render_graph_initialize
(
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  crude_linear_allocator_initialize( &render_graph->local_allocator, CRUDE_RMEGA( 1u ), "RenderGraphLocalAllocator" );
  render_graph->local_allocator_container = crude_linear_allocator_pack( &render_graph->local_allocator );
}

void
crude_gfx_render_graph_parse_from_file
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ char const                                         *file_path,
  _In_ crude_stack_allocator                              *temp_allocator
)
{
  crude_string_buffer                                      string_buffer;
  crude_allocator_container                                temp_allocator_container;
  cJSON                                                   *render_graph_json;
  cJSON const                                             *passes;
  cJSON const                                             *pass;
  uint8                                                   *render_graph_json_buffer;
  uint32                                                   render_graph_json_buffer_size, temp_allocator_container_maker;

  temp_allocator_container = crude_stack_allocator_pack( temp_allocator );
  temp_allocator_container_maker = crude_stack_allocator_get_marker( temp_allocator );
  
  crude_string_buffer_initialize( &string_buffer, 1024, render_graph->local_allocator_container );

  if ( !crude_file_exist( file_path ) )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse render graph", file_path );
    return;
  }

  crude_read_file( file_path, temp_allocator_container, &render_graph_json_buffer, &render_graph_json_buffer_size );

  render_graph_json = cJSON_ParseWithLength( render_graph_json_buffer, render_graph_json_buffer_size );
  if ( !render_graph_json )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file \"%s\" for render graph... Error %s", file_path, cJSON_GetErrorPtr() );
    return;
  }

  passes = cJSON_GetObjectItemCaseSensitive( render_graph_json, "passes" );
  pass = NULL;
  cJSON_ArrayForEach( pass, passes )
  {
    crude_gfx_render_graph_node_creation                   node_creation;
    cJSON const                                           *pass_inputs;
    cJSON const                                           *pass_outputs;
    cJSON const                                           *pass_input;
    cJSON const                                           *pass_output;
    cJSON const                                           *pass_name;
    cJSON const                                           *pass_enabled;

    pass_inputs = cJSON_GetObjectItemCaseSensitive( pass, "inputs" );
    pass_outputs = cJSON_GetObjectItemCaseSensitive( pass, "outputs" );
    pass_name = cJSON_GetObjectItemCaseSensitive( pass, "name" );
    pass_enabled = cJSON_GetObjectItemCaseSensitive( pass, "enabled" );
    CRUDE_ASSERT( pass_inputs && pass_outputs && pass_name && pass_enabled );

    node_creation = ( crude_gfx_render_graph_node_creation ){ 0 };
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( node_creation.inputs, cJSON_GetArraySize( pass_inputs ), temp_allocator_container );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( node_creation.outputs, cJSON_GetArraySize( pass_outputs ), temp_allocator_container );
  
    pass_input = NULL;
    cJSON_ArrayForEach( pass_input, pass_inputs )
    {
      cJSON const                                         *input_type;
      cJSON const                                         *input_name;

      input_type = cJSON_GetObjectItemCaseSensitive( pass_input, "type" );
      input_name = cJSON_GetObjectItemCaseSensitive( pass_input, "name" );
      CRUDE_ASSERT( input_type && input_name );
      
      CRUDE_ARRAY_PUSH( node_creation.inputs, ( ( crude_gfx_render_graph_resource_input_creation ){ 
        .type = _string_to_resource_type( cJSON_GetStringValue( input_name ) ),
        .resource_info.external = false,
        .name = crude_string_buffer_append_use_f( &string_buffer, "%s", cJSON_GetStringValue( input_name ) ),
      } ) );
    }
    
    pass_output = NULL;
    cJSON_ArrayForEach( pass_output, pass_outputs )
    {
      cJSON const                                         *output_type;
      cJSON const                                         *output_name;
      crude_gfx_render_graph_resource_output_creation      output_creation;
      
      output_type = cJSON_GetObjectItemCaseSensitive( pass_output, "type" );
      output_name = cJSON_GetObjectItemCaseSensitive( pass_output, "name" );
      CRUDE_ASSERT( output_type && output_name );

      output_creation = ( crude_gfx_render_graph_resource_output_creation ){ 0 };
      output_creation.type = _string_to_resource_type( cJSON_GetStringValue( output_type ) );
      output_creation.name = crude_string_buffer_append_use_f( &string_buffer, "%s", cJSON_GetStringValue( output_name ) );

      switch ( output_creation.type )
      {
        case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT:
        case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE:
        {
          cJSON const                                     *output_format;
          cJSON const                                     *output_load_op;
          cJSON const                                     *output_resolution;
          
          output_format = cJSON_GetObjectItemCaseSensitive( pass_output, "format" );
          output_load_op = cJSON_GetObjectItemCaseSensitive( pass_output, "op" );
          output_resolution = cJSON_GetObjectItemCaseSensitive( pass_output, "resolution" );
          CRUDE_ASSERT( output_format && output_load_op && output_resolution );
          CRUDE_ASSERT( cJSON_GetArraySize( output_resolution ) == 2 );

          output_creation.resource_info.texture.format = crude_string_to_vk_format( cJSON_GetStringValue( output_format ) );
          output_creation.resource_info.texture.load_op = string_to_render_pass_operation( cJSON_GetStringValue( output_load_op ) );
          output_creation.resource_info.texture.width = cJSON_GetArrayItem( output_resolution, 0 );
          output_creation.resource_info.texture.height = cJSON_GetArrayItem( output_resolution, 1 );
          output_creation.resource_info.texture.depth = 1;
          break;
        }
        case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_BUFFER:
        {
          CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "TODO" );
          break;
        }
      }
        
      CRUDE_ARRAY_PUSH( node_creation.outputs, output_creation );
    }

    node_creation.name = crude_string_buffer_append_use_f( &string_buffer, "%s", cJSON_GetStringValue( pass_name ) );
    node_creation.enabled = cJSON_GetNumberValue( pass_enabled );
    
    crude_gfx_render_graph_node_handle node_handle = builder->create_node( node_creation );
    CRUDE_ARRAY_PUSH( render_graph->nodes, node_handle );
  }

  crude_stack_allocator_free_marker( temp_allocator, temp_allocator_container_maker );
}