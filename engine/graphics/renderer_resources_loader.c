#include <cJSON.h>

#include <core/file.h>
#include <core/string.h>

#include <graphics/renderer_resources_loader.h>


void parse_gpu_pipeline
(
  cJSON const                                             *pipeline_json,
  crude_gfx_pipeline_creation                             *pipeline_creation,
  crude_string_buffer                                     *path_buffer,
  crude_gfx_render_graph                                  *render_graph,
  crude_stack_allocator                                   *temporary_allocator
);

void
crude_gfx_renderer_resource_load_technique
(
  _In_ char const                                         *json_name,
  crude_gfx_renderer                                      *renderer,
  crude_gfx_render_graph                                  *render_graph,
  crude_stack_allocator                                   *temporary_allocator
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
  json_path = crude_string_buffer_append_use_f( &path_buffer, "%s%s", working_directory, json_name );;
  if ( !crude_file_exist( json_path ) )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse render graph", json_path );
    return;
  }
  
  crude_read_file( json_path, crude_stack_allocator_pack( temporary_allocator ), &technique_json_buffer, &technique_json_buffer_size );

  technique_json = cJSON_ParseWithLength( technique_json_buffer, technique_json_buffer_size );
  if ( !technique_json )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file for technique... Error %s", cJSON_GetErrorPtr() );
    return;
  }
  
  technique_creation = ( crude_gfx_renderer_technique_creation ){ 0 };

  {
    cJSON const                                           *technique_name_json;
    char const                                            *technique_name;
    
    technique_name_json = cJSON_GetObjectItemCaseSensitive( technique_json, "name" );
    technique_name = cJSON_GetStringValue( technique_name_json );
    // !TODO unsafe
    technique_creation.name = technique_name;
  }

  {
    cJSON const                                           *pipelines_json;

    pipelines_json = cJSON_GetObjectItemCaseSensitive( technique_json, "pipelines" );
    if ( cJSON_GetArraySize( pipelines_json ) > 0 )
    {
      uint32                                               pipelines_num;
      
      pipelines_num = cJSON_GetArraySize( pipelines_json );
      for ( uint32 i = 0; i < pipelines_num; ++i )
      {
        cJSON const                                       *pipeline;
        crude_gfx_pipeline_creation                        pipeline_creation;

        pipeline = cJSON_GetArrayItem( pipelines_json, i );
        pipeline_creation = crude_gfx_pipeline_creation_empty();
        parse_gpu_pipeline( pipeline, &pipeline_creation, &path_buffer, render_graph, temporary_allocator );
        technique_creation.creations[ technique_creation.num_creations++ ] = pipeline_creation;
      }
    }
  }

  {
    crude_gfx_renderer_technique *technique = crude_gfx_renderer_create_technique( renderer, &technique_creation );
  }
  
  cJSON_Delete( technique_json );
  crude_stack_allocator_free_marker( temporary_allocator, allocated_marker  );
}

VkBlendFactor
get_blend_factor( char const *factor )
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
get_blend_op
(
  char const *op
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

void parse_gpu_pipeline
(
  cJSON const                                             *pipeline_json,
  crude_gfx_pipeline_creation                             *pipeline_creation,
  crude_string_buffer                                     *path_buffer,
  crude_gfx_render_graph                                  *render_graph,
  crude_stack_allocator                                   *temporary_allocator
)
{
  char working_directory[ 512 ];
  crude_get_current_working_directory( working_directory, sizeof( working_directory ) );

  cJSON const *shaders_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "shaders" );
  if ( shaders_json != NULL )
  {
    for ( size_t shader_index = 0; shader_index < cJSON_GetArraySize( shaders_json ); ++shader_index )
    {
      cJSON const *shader_stage_json = cJSON_GetArrayItem( shaders_json, shader_index );

      crude_string_buffer_clear( path_buffer );

      uint32 code_size;
      char const *code;
      const char *shader_filename = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( shader_stage_json, "shader" ) );
      const char *shader_path = crude_string_buffer_append_use_f( path_buffer, "%s%s%s", working_directory, "\\..\\..\\shaders\\", shader_filename );
      crude_read_file( shader_path, crude_stack_allocator_pack( temporary_allocator ), &code, &code_size );
      
      char const *name = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( shader_stage_json, "stage" ) );
      
      if ( strcmp( name, "vertex" ) == 0 )
      {
        crude_shader_state_creation_add_state( &pipeline_creation->shaders, code, strlen( code ), VK_SHADER_STAGE_VERTEX_BIT );
      }
      else if ( strcmp( name, "fragment" ) == 0 )
      {
        crude_shader_state_creation_add_state( &pipeline_creation->shaders, code, strlen( code ), VK_SHADER_STAGE_FRAGMENT_BIT );
      }
      else if ( strcmp( name, "compute" ) == 0 )
      {
        crude_shader_state_creation_add_state( &pipeline_creation->shaders, code, strlen( code ), VK_SHADER_STAGE_COMPUTE_BIT );
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
      
      char const *enabled = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( blend_json, "enable" ) );
      char const *src_colour = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( blend_json, "src_colour" ) );
      char const *dst_colour = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( blend_json, "dst_colour" ) );
      char const *blend_op = cJSON_GetStringValue( cJSON_GetObjectItemCaseSensitive( blend_json, "op" ) );
      
      crude_gfx_blend_state blend_state = {
        .blend_enabled = ( strcmp( enabled, "true" ) == 0 ),
        .source_color = get_blend_factor( src_colour ),
        .destination_color = get_blend_factor( dst_colour ),
        .color_operation = get_blend_op( blend_op ),
      };
      pipeline_creation->blend_state.blend_states[ pipeline_creation->blend_state.active_states++ ] = blend_state;
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
    else
    {
      CRUDE_ASSERT( false );
    }
  }
  
  cJSON const *render_pass_json = cJSON_GetObjectItemCaseSensitive( pipeline_json, "render_pass" );
  if ( render_pass_json != NULL )
  {
    char const *render_pass_name = cJSON_GetStringValue( render_pass_json );
    
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
}