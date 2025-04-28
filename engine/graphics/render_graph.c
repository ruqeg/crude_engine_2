#include <cJSON.h>

#include <core/file.h>
#include <core/log.h>
#include <core/assert.h>

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


void
crude_gfx_render_graph_parse_from_file
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ char const                                         *file_path,
  _In_ crude_allocator                                     temp_allocator
)
{
  if ( !crude_file_exist( file_path ) )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse render graph", file_path );
    return;
  }

  uint8 *buffer;
  uint32 buffer_size;
  crude_read_file( file_path, temp_allocator, &buffer, &buffer_size );

  cJSON *graph_json = cJSON_ParseWithLength( buffer, buffer_size );
  if ( !graph_json )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file \"%s\" for render graph... Error %s", file_path, cJSON_GetErrorPtr() );
    return;
  }

  cJSON const *passes = cJSON_GetObjectItemCaseSensitive( graph_json, "passes" );
  cJSON const *pass = NULL;
  
  cJSON_ArrayForEach( pass, passes )
  {
    cJSON const *pass_inputs = cJSON_GetObjectItemCaseSensitive( pass, "inputs" );
    cJSON const *pass_outputs = cJSON_GetObjectItemCaseSensitive( pass, "outputs" );

    crude_gfx_render_graph_node_creation node_creation = { 0 };
    CRUDE_ARR_SETLEN( node_creation.inputs, cJSON_GetArraySize( pass_inputs ) );
    CRUDE_ARR_SETLEN( node_creation.outputs, cJSON_GetArraySize( pass_outputs ) );
  
    uint32 input_index = 0u;
    cJSON const *pass_input = NULL;
    cJSON_ArrayForEach( pass_input, pass_inputs )
    {
      cJSON const *input_type = cJSON_GetObjectItemCaseSensitive( pass, "type" );
      cJSON const *input_name = cJSON_GetObjectItemCaseSensitive( pass, "name" );
      CRUDE_ASSERT( input_type && input_name );
      
      node_creation.inputs[ input_index++ ] = ( crude_gfx_render_graph_resource_input_creation ){
        .type = _string_to_resource_type( input_name->string ),
        .resource_info.external = false,
        .name = string_buffer.append_use_f( "%s", input_name.c_str() ),
      }; 
    }

        for ( sizet oi = 0; oi < pass_outputs.size(); ++oi ) {
            json pass_output = pass_outputs[ oi ];

            FrameGraphResourceOutputCreation output_creation{ };

            std::string output_type = pass_output.value( "type", "" );
            RASSERT( !output_type.empty() );

            std::string output_name = pass_output.value( "name", "" );
            RASSERT( !output_name.empty() );

            output_creation.type = string_to_resource_type( output_type.c_str() );

            output_creation.name = string_buffer.append_use_f( "%s", output_name.c_str() );

            switch ( output_creation.type ) {
                case FrameGraphResourceType_Attachment:
                case FrameGraphResourceType_Texture:
                {
                    std::string format = pass_output.value( "format", "" );
                    RASSERT( !format.empty() );

                    output_creation.resource_info.texture.format = util_string_to_vk_format( format.c_str() );

                    std::string load_op = pass_output.value( "op", "" );
                    RASSERT( !load_op.empty() );

                    output_creation.resource_info.texture.load_op = string_to_render_pass_operation( load_op.c_str() );

                    json resolution = pass_output[ "resolution" ];

                    output_creation.resource_info.texture.width = resolution[0];
                    output_creation.resource_info.texture.height = resolution[1];
                    output_creation.resource_info.texture.depth = 1;
                } break;
                case FrameGraphResourceType_Buffer:
                {
                    // TODO(marco)
                    RASSERT( false );
                } break;
            }

            node_creation.outputs.push( output_creation );
        }

        name_value = pass.value( "name", "" );
        RASSERT( !name_value.empty() );

        bool enabled = pass.value( "enabled", true );

        node_creation.name = string_buffer.append_use_f( "%s", name_value.c_str() );
        node_creation.enabled = enabled;

        FrameGraphNodeHandle node_handle = builder->create_node( node_creation );
        nodes.push( node_handle );
  }

  //temp_allocator->free_marker( current_allocator_marker );
}