#include <cJSON.h>

#include <core/file.h>
#include <core/string.h>

#include <graphics/renderer_resources_loader.h>

/************************************************
 *
 * Local Renderer Resources Loader Functions Declaration.
 * 
 ***********************************************/
static void
load_shader_to_string_buffer_
(
  _In_ char const                                         *shader_filename,
  _In_ char const                                         *working_directory,
  _In_ uint32                                             *total_code_size,
  _In_ crude_string_buffer                                *shader_code_buffer,
  _In_ crude_string_buffer                                *path_buffer,
  _In_ crude_stack_allocator                              *temporary_allocator
);

static void
parse_gpu_pipeline_
(
  _In_ cJSON const                                        *pipeline_json,
  _Out_ crude_gfx_pipeline_creation                       *pipeline_creation,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_string_buffer                                *path_buffer,
  _In_ crude_string_buffer                                *shader_code_buffer,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
);

static VkBlendFactor
get_blend_factor_
(
  _In_ char const                                         *factor
);

static VkBlendOp
get_blend_op_
(
  _In_ char const                                         *op
);

/************************************************
 *
 * Renderer Technique
 * 
 ***********************************************/
void
crude_gfx_renderer_technique_load_from_file
(
  _In_ char const                                         *json_name,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  char                                                     working_directory[ 512 ];
  char const                                              *json_path;
  crude_gfx_renderer_technique_creation                    technique_creation;
  cJSON                                                   *technique_json;
  cJSON const                                             *passes;
  uint8                                                   *technique_json_buffer;
  crude_string_buffer                                      shader_code_buffer;
  crude_string_buffer                                      path_buffer;
  size_t                                                   allocated_marker;
  uint32                                                   technique_json_buffer_size;

  allocated_marker = crude_stack_allocator_get_marker( temporary_allocator );
  
  crude_string_buffer_initialize( &shader_code_buffer, CRUDE_RKILO( 64 ), crude_stack_allocator_pack( temporary_allocator ) );
  crude_string_buffer_initialize( &path_buffer, 1024, crude_stack_allocator_pack( temporary_allocator ) );

  crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
  json_path = crude_string_buffer_append_use_f( &renderer->gpu->objects_names_string_buffer, "%s%s", working_directory, json_name );;
  if ( !crude_file_exist( json_path ) )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse render graph", json_path );
    return;
  }
  
  crude_read_file( json_path, crude_stack_allocator_pack( temporary_allocator ), &technique_json_buffer, &technique_json_buffer_size );

  technique_json = cJSON_ParseWithLength( CRUDE_REINTERPRET_CAST( char const*, technique_json_buffer ), technique_json_buffer_size );
  if ( !technique_json )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file for technique... Error %s", cJSON_GetErrorPtr() );
    return;
  }
  
  technique_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_renderer_technique_creation );
  
  technique_creation.json_name = json_name;
  {
    cJSON const                                           *technique_name_json;
    char const                                            *technique_name;
    
    technique_name_json = cJSON_GetObjectItemCaseSensitive( technique_json, "name" );
    technique_name = cJSON_GetStringValue( technique_name_json );
    technique_creation.name = crude_string_buffer_append_use_f( &renderer->gpu->objects_names_string_buffer, "%s", technique_name );
  }
  {
    cJSON const                                           *pipelines_json;

    pipelines_json = cJSON_GetObjectItemCaseSensitive( technique_json, "pipelines" );
    for ( uint32 i = 0; i < cJSON_GetArraySize( pipelines_json ); ++i )
    {
      cJSON const                                       *pipeline;
      crude_gfx_pipeline_creation                        pipeline_creation;

      pipeline = cJSON_GetArrayItem( pipelines_json, i );
      pipeline_creation = crude_gfx_pipeline_creation_empty();
      parse_gpu_pipeline_( pipeline, &pipeline_creation, renderer->gpu, &path_buffer, &shader_code_buffer, render_graph, temporary_allocator );
      crude_gfx_renderer_technique_creation_add_pass( &technique_creation, crude_gfx_create_pipeline( renderer->gpu, &pipeline_creation ) );
      crude_string_buffer_clear( &shader_code_buffer );
      crude_string_buffer_clear( &path_buffer );
    }
  }

  {
    crude_gfx_renderer_technique *technique = crude_gfx_renderer_create_technique( renderer, &technique_creation );
  }
  
  cJSON_Delete( technique_json );
  crude_stack_allocator_free_marker( temporary_allocator, allocated_marker  );
}

/************************************************
 *
 * Local Renderer Resources Loader Functions Implementation.
 * 
 ***********************************************/
void
parse_gpu_pipeline_
(
  _In_ cJSON const                                        *pipeline_json,
  _Out_ crude_gfx_pipeline_creation                       *pipeline_creation,
  _In_ crude_gfx_device                                   *gpu,
  _In_ crude_string_buffer                                *path_buffer,
  _In_ crude_string_buffer                                *shader_code_buffer,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  char                                                     working_directory[ 512 ];

  crude_get_current_working_directory( working_directory, sizeof( working_directory ) );

  cJSON const *shaders_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "shaders" );
  if ( shaders_json != NULL )
  {
    for ( size_t shader_index = 0; shader_index < cJSON_GetArraySize( shaders_json ); ++shader_index )
    {
      cJSON const                                         *shader_stage_json;
      cJSON const                                         *includes;
      char const                                          *stage;
      char                                                *total_code;
      char const                                          *shader_filename;
      char const                                          *shader_path;
      uint32                                               total_code_size;
      
      crude_string_buffer_clear( path_buffer );

      total_code = crude_string_buffer_current( shader_code_buffer );
      total_code_size = 0u;

      shader_stage_json = cJSON_GetArrayItem( shaders_json, shader_index );
      includes = cJSON_GetObjectItemCaseSensitive( shader_stage_json, "includes" );

      if ( cJSON_IsArray( includes ) )
      {
        for ( size_t include_index = 0; include_index < cJSON_GetArraySize( includes ); ++include_index )
        {
          shader_filename = cJSON_GetStringValue( cJSON_GetArrayItem( includes, include_index ) );
          load_shader_to_string_buffer_( shader_filename, working_directory, &total_code_size, shader_code_buffer, path_buffer, temporary_allocator );
        }
      }
      
      shader_filename = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( shader_stage_json, "shader" ) );
      load_shader_to_string_buffer_( shader_filename, working_directory, &total_code_size, shader_code_buffer, path_buffer, temporary_allocator );

      stage = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( shader_stage_json, "stage" ) );
      if ( strcmp( stage, "vertex" ) == 0 )
      {
        crude_gfx_shader_state_creation_add_stage( &pipeline_creation->shaders, total_code, total_code_size, VK_SHADER_STAGE_VERTEX_BIT );
      }
      else if ( strcmp( stage, "fragment" ) == 0 )
      {
        crude_gfx_shader_state_creation_add_stage( &pipeline_creation->shaders, total_code, total_code_size, VK_SHADER_STAGE_FRAGMENT_BIT );
      }
      else if ( strcmp( stage, "compute" ) == 0 )
      {
        crude_gfx_shader_state_creation_add_stage( &pipeline_creation->shaders, total_code, total_code_size, VK_SHADER_STAGE_COMPUTE_BIT );
      }
      else if ( strcmp( stage, "mesh" ) == 0 )
      {
        crude_gfx_shader_state_creation_add_stage( &pipeline_creation->shaders, total_code, total_code_size, VK_SHADER_STAGE_MESH_BIT_EXT );
      }
      else if ( strcmp( stage, "task" ) == 0 )
      {
        crude_gfx_shader_state_creation_add_stage( &pipeline_creation->shaders, total_code, total_code_size, VK_SHADER_STAGE_TASK_BIT_EXT );
      }
    }
  }
  
  cJSON const *depth_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "depth" );
  if ( depth_json != NULL )
  {
    pipeline_creation->depth_stencil.depth_enable = 1;
    pipeline_creation->depth_stencil.depth_write_enable = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( depth_json, "write" ) );
    
    cJSON const *comparison_json = cJSON_GetObjectItemCaseSensitive( depth_json, "test" );
    if ( cJSON_IsString( comparison_json ) )
    {
      char const *comparison = cJSON_GetStringValue( comparison_json );
      
      if ( strcmp( comparison, "less_or_equal" ) == 0 )
      {
        pipeline_creation->depth_stencil.depth_comparison = VK_COMPARE_OP_LESS_OR_EQUAL;
      }
      else if ( strcmp( comparison, "equal" ) == 0 )
      {
        pipeline_creation->depth_stencil.depth_comparison = VK_COMPARE_OP_EQUAL;
      }
      else if ( strcmp( comparison, "never" ) == 0 )
      {
        pipeline_creation->depth_stencil.depth_comparison = VK_COMPARE_OP_NEVER;
      }
      else if ( strcmp( comparison, "always" ) == 0 )
      {
        pipeline_creation->depth_stencil.depth_comparison = VK_COMPARE_OP_ALWAYS;
      }
      else
      {
        CRUDE_ASSERT( false );
      }
    }
  }
  
  cJSON const *blend_states_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "blend" );
  if ( blend_states_json != NULL )
  {
    for ( size_t blend_index = 0; blend_index < cJSON_GetArraySize( blend_states_json ); ++blend_index )
    {
      cJSON const *blend_json = cJSON_GetArrayItem( blend_states_json, blend_index );
      
      uint32 enabled = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( blend_json, "enable" ) );
      char const *src_colour = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( blend_json, "src_colour" ) );
      char const *dst_colour = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( blend_json, "dst_colour" ) );
      char const *blend_op = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( blend_json, "op" ) );
      
      crude_gfx_blend_state blend_state = CRUDE_COMPOUNT_EMPTY( crude_gfx_blend_state );
      blend_state.source_color = get_blend_factor_( src_colour );
      blend_state.destination_color = get_blend_factor_( dst_colour );
      blend_state.color_operation = get_blend_op_( blend_op );
      blend_state.blend_enabled = enabled;
      crude_gfx_pipeline_creation_add_blend_state( pipeline_creation, blend_state );
    }
  }
  
  cJSON const *cull_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "cull" );
  if ( cull_json != NULL )
  {
    char const *cull_mode = cJSON_GetStringValue( cull_json );

    if ( strcmp( cull_mode, "back" ) == 0 )
    {
      pipeline_creation->rasterization.cull_mode = VK_CULL_MODE_BACK_BIT;
    }
    else if ( strcmp( cull_mode, "front" ) == 0 )
    {
      pipeline_creation->rasterization.cull_mode = VK_CULL_MODE_FRONT_BIT;
    }
    else
    {
      CRUDE_ASSERT( false );
    }
  }
  
  cJSON const *topology_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "topology" );
  if ( topology_json != NULL )
  {
    char const *topology_str = cJSON_GetStringValue( topology_json );
    pipeline_creation->topology = crude_gfx_string_to_vk_primitive_topology( topology_str );
  }

  pipeline_creation->rasterization.front = VK_FRONT_FACE_CLOCKWISE;
  
  cJSON const *render_pass_output_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "render_pass_output" );
  if ( render_pass_output_json != NULL )
  {
    cJSON const *render_pass_output_reference_json = cJSON_GetObjectItemCaseSensitive( render_pass_output_json, "reference" );
    cJSON const *render_pass_output_custom_json = cJSON_GetObjectItemCaseSensitive( render_pass_output_json, "custom" );
    if ( render_pass_output_reference_json )
    {
      char const *render_pass_name = cJSON_GetStringValue( render_pass_output_reference_json );
    
      crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node_by_name( render_graph->builder, render_pass_name );
    
      if ( node )
      {
        if ( strcmp( render_pass_name, "swapchain" ) == 0 )
        {
          pipeline_creation->render_pass_output = render_graph->builder->gpu->swapchain_output;
        }
        else
        {
          crude_gfx_render_pass const *render_pass = crude_gfx_access_render_pass( render_graph->builder->gpu, node->render_pass );
          pipeline_creation->render_pass_output = render_pass->output;
        }
      }
      else
      {
        CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find render pass %s. Defaulting to swapchain", render_pass_name );
        pipeline_creation->render_pass_output = render_graph->builder->gpu->swapchain_output;
      }
    }
    else if ( render_pass_output_custom_json )
    {
      pipeline_creation->render_pass_output = crude_gfx_render_pass_output_empty( );

      for ( uint32 i = 0; i < cJSON_GetArraySize( render_pass_output_custom_json ); ++i )
      {
        cJSON const *render_pass_output_custom_attachment_json = cJSON_GetArrayItem( render_pass_output_custom_json, i );
        cJSON const *render_pass_output_custom_attachment_format_json = cJSON_GetObjectItemCaseSensitive( render_pass_output_custom_attachment_json, "format" );
        cJSON const *render_pass_output_custom_attachment_load_op_json = cJSON_GetObjectItemCaseSensitive( render_pass_output_custom_attachment_json, "op" );

        VkFormat vk_format = crude_gfx_string_to_vk_format( cJSON_GetStringValue( render_pass_output_custom_attachment_format_json ) );
        crude_gfx_render_pass_operation operation = crude_gfx_string_to_render_pass_operation( cJSON_GetStringValue( render_pass_output_custom_attachment_load_op_json ) );

        if ( crude_gfx_has_depth_or_stencil( vk_format ) )
        {
          crude_gfx_render_pass_output_set_depth( &pipeline_creation->render_pass_output, vk_format, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, operation, CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE );
        }
        else
        {
          crude_gfx_render_pass_output_add_color( &pipeline_creation->render_pass_output, crude_gfx_string_to_vk_format( cJSON_GetStringValue( render_pass_output_custom_attachment_format_json ) ), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, operation );
        }
      }
    }
  }

  cJSON const *vertex_input_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "vertex_input" );
  if ( vertex_input_json != NULL )
  {
    cJSON const *vertex_attributes_json = cJSON_GetObjectItemCaseSensitive( vertex_input_json, "attributes" );
    for ( size_t i = 0; i < cJSON_GetArraySize( vertex_attributes_json ); ++i )
    {
      cJSON const *vertex_attribute_json = cJSON_GetArrayItem( vertex_attributes_json, i );
      uint32 location = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( vertex_attribute_json, "location" ) );
      uint32 binding = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( vertex_attribute_json, "binding" ) );
      uint32 offset = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( vertex_attribute_json, "offset" ) );
      crude_gfx_vertex_component_format format = crude_gfx_to_vertex_component_format( cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( vertex_attribute_json, "format" ) ) );
      crude_gfx_pipeline_creation_add_vertex_attribute( pipeline_creation, location, binding, offset, format );
    }

    cJSON const *vertex_streams_json = cJSON_GetObjectItemCaseSensitive( vertex_input_json, "streams" );
    for ( size_t i = 0; i < cJSON_GetArraySize( vertex_streams_json ); ++i )
    {
      cJSON const *vertex_stream_json = cJSON_GetArrayItem( vertex_streams_json, i );
      uint32 binding = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( vertex_stream_json, "binding" ) );
      uint32 stride = cJSON_GetNumberValue( cJSON_GetObjectItemCaseSensitive( vertex_stream_json, "stride" ) );
      char const *input_rate = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( vertex_stream_json, "input_rate" ) );
      crude_gfx_pipeline_creation_add_vertex_stream( pipeline_creation, binding, stride, crude_string_cmp( input_rate, "instance" ) ?  CRUDE_GFX_VERTEX_INPUT_RATE_PER_VERTEX : CRUDE_GFX_VERTEX_INPUT_RATE_PER_INSTANCE );
    }
  }
  
  cJSON const *name_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "name" );
  if ( name_json != NULL )
  {
    pipeline_creation->name = cJSON_GetStringValue( name_json );
  }
}

void
load_shader_to_string_buffer_
(
  _In_ char const                                         *shader_filename,
  _In_ char const                                         *working_directory,
  _In_ uint32                                             *total_code_size,
  _In_ crude_string_buffer                                *shader_code_buffer,
  _In_ crude_string_buffer                                *path_buffer,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  uint8                                                   *code;
  char const                                              *shader_path;
  uint32                                                   temporary_allocator_marker, code_size;

  temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );
  
  shader_path = crude_string_buffer_append_use_f( path_buffer, "%s%s%s", working_directory, "\\..\\..\\shaders\\", shader_filename );
  crude_read_file( shader_path, crude_stack_allocator_pack( temporary_allocator ), CRUDE_REINTERPRET_CAST( uint8**, &code ), &code_size );
  crude_string_buffer_append_m( shader_code_buffer, code, code_size );
  *total_code_size += code_size;

  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
}

VkBlendFactor
get_blend_factor_
(
  _In_ char const                                         *factor
)
{
  if ( strcmp( factor, "ZERO" ) == 0 )
  {
    return VK_BLEND_FACTOR_ZERO;
  }
  if ( strcmp( factor, "ONE" ) == 0 )
  {
    return VK_BLEND_FACTOR_ONE;
  }
  if ( strcmp( factor, "SRC_COLOR" ) == 0 )
  {
    return VK_BLEND_FACTOR_SRC_COLOR;
  }
  if ( strcmp( factor, "ONE_MINUS_SRC_COLOR" ) == 0 )
  {
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  }
  if ( strcmp( factor, "DST_COLOR" ) == 0 )
  {
    return VK_BLEND_FACTOR_DST_COLOR;
  }
  if ( strcmp( factor, "ONE_MINUS_DST_COLOR" ) == 0 )
  {
    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
  }
  if ( strcmp( factor, "SRC_ALPHA" ) == 0 )
  {
    return VK_BLEND_FACTOR_SRC_ALPHA;
  }
  if ( strcmp( factor, "ONE_MINUS_SRC_ALPHA" ) == 0 )
  {
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  }
  if ( strcmp( factor, "DST_ALPHA" ) == 0 )
  {
    return VK_BLEND_FACTOR_DST_ALPHA;
  }
  if ( strcmp( factor, "ONE_MINUS_DST_ALPHA" ) == 0 )
  {
    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  }
  if ( strcmp( factor, "CONSTANT_COLOR" ) == 0 )
  {
    return VK_BLEND_FACTOR_CONSTANT_COLOR;
  }
  if ( strcmp( factor, "ONE_MINUS_CONSTANT_COLOR" ) == 0 )
  {
    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
  }
  if ( strcmp( factor, "CONSTANT_ALPHA" ) == 0 )
  {
    return VK_BLEND_FACTOR_CONSTANT_ALPHA;
  }
  if ( strcmp( factor, "ONE_MINUS_CONSTANT_ALPHA" ) == 0 )
  {
    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
  }
  if ( strcmp( factor, "SRC_ALPHA_SATURATE" ) == 0 )
  {
    return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
  }
  if ( strcmp( factor, "SRC1_COLOR" ) == 0 )
  {
    return VK_BLEND_FACTOR_SRC1_COLOR;
  }
  if ( strcmp( factor, "ONE_MINUS_SRC1_COLOR" ) == 0 )
  {
    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
  }
  if ( strcmp( factor, "SRC1_ALPHA" ) == 0 )
  {
    return VK_BLEND_FACTOR_SRC1_ALPHA;
  }
  if ( strcmp( factor, "ONE_MINUS_SRC1_ALPHA" ) == 0 )
  {
    return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
  }
  
  return VK_BLEND_FACTOR_ONE;
}

VkBlendOp
get_blend_op_
(
  _In_ char const                                         *op
)
{
  if ( strcmp( op, "ADD" ) == 0 )
  {
    return VK_BLEND_OP_ADD;
  }
  if ( strcmp( op, "SUBTRACT" ) == 0 )
  {
    return VK_BLEND_OP_SUBTRACT;
  }
  if ( strcmp( op, "REVERSE_SUBTRACT" ) == 0 )
  {
    return VK_BLEND_OP_REVERSE_SUBTRACT;
  }
  if ( strcmp( op, "MIN" ) == 0 )
  {
    return VK_BLEND_OP_MIN;
  }
  if ( strcmp( op, "MAX" ) == 0 )
  {
    return VK_BLEND_OP_MAX;
  }
  
  return VK_BLEND_OP_ADD;
}