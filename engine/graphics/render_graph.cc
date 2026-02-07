#include <thirdparty/cJSON/cJSON.h>

#include <engine/core/file.h>
#include <engine/core/log.h>
#include <engine/core/assert.h>
#include <engine/core/hash_map.h>
#include <engine/core/string.h>
#include <engine/core/profiler.h>

#include <engine/graphics/render_graph.h>

/************************************************
 *
 * Render Graph Functions
 * 
 ***********************************************/
void
crude_gfx_render_graph_initialize
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  crude_linear_allocator_initialize( &render_graph->linear_allocator, CRUDE_RMEGA( 1u ), "render_graph_linear_allocator" );
  render_graph->builder = builder;

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( render_graph->nodes, CRUDE_GRAPHICS_RENDER_GRAPH_MAX_NODES_COUNT, crude_linear_allocator_pack( &render_graph->linear_allocator ) ); 
}

void
crude_gfx_render_graph_deinitialize
(
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++i )
  {
    crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ i ] );
    
    crude_gfx_destroy_render_pass( render_graph->builder->gpu, node->render_pass );
    crude_gfx_destroy_framebuffer( render_graph->builder->gpu, node->framebuffer );
    
    CRUDE_ARRAY_DEINITIALIZE( node->inputs ); 
    CRUDE_ARRAY_DEINITIALIZE( node->outputs ); 
    CRUDE_ARRAY_DEINITIALIZE( node->edges ); 
  }
  
  CRUDE_ARRAY_DEINITIALIZE( render_graph->nodes ); 

  crude_linear_allocator_deinitialize( &render_graph->linear_allocator );
}

void
crude_gfx_render_graph_parse_from_file
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ char const                                         *render_graph_absolute_filepath,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  crude_string_buffer                                      temporary_string_buffer;
  cJSON                                                   *render_graph_json;
  cJSON const                                             *passes;
  cJSON const                                             *pass;
  uint8                                                   *render_graph_json_buffer;
  uint32                                                   render_graph_json_buffer_size, temporary_allocator_maker;
  
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Parse render graph \"%s\"", render_graph_absolute_filepath );

  if ( !crude_file_exist( render_graph_absolute_filepath ) )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot find a file \"%s\" to parse render graph", render_graph_absolute_filepath );
    return;
  }
  
  temporary_allocator_maker = crude_stack_allocator_get_marker( temporary_allocator );

  crude_read_file( render_graph_absolute_filepath, crude_stack_allocator_pack( temporary_allocator ), &render_graph_json_buffer, &render_graph_json_buffer_size );
  render_graph_json = cJSON_ParseWithLength( CRUDE_REINTERPRET_CAST( char const*, render_graph_json_buffer ), render_graph_json_buffer_size );
  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_maker );
    
  if ( !render_graph_json )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Cannot parse a file \"%s\" for render graph... Error %s", render_graph_absolute_filepath, cJSON_GetErrorPtr() );
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
    cJSON const                                           *pass_pipeline_type;
    
  
    temporary_allocator_maker = crude_stack_allocator_get_marker( temporary_allocator );
    
    crude_string_buffer_initialize( &temporary_string_buffer, CRUDE_RKILO( 4 ), crude_stack_allocator_pack( temporary_allocator ) );

    pass_inputs = cJSON_GetObjectItemCaseSensitive( pass, "inputs" );
    pass_outputs = cJSON_GetObjectItemCaseSensitive( pass, "outputs" );
    CRUDE_ASSERT( pass_outputs );

    node_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_render_graph_node_creation );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( node_creation.inputs, cJSON_GetArraySize( pass_inputs ), crude_stack_allocator_pack( temporary_allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( node_creation.outputs, cJSON_GetArraySize( pass_outputs ), crude_stack_allocator_pack( temporary_allocator ) );
  
    if ( pass_inputs )
    {
      pass_input = NULL;
      cJSON_ArrayForEach( pass_input, pass_inputs )
      {
        cJSON const                                         *input_type;
        cJSON const                                         *input_name;
        crude_gfx_render_graph_resource_input_creation       creation;

        input_type = cJSON_GetObjectItemCaseSensitive( pass_input, "type" );
        input_name = cJSON_GetObjectItemCaseSensitive( pass_input, "name" );
        CRUDE_ASSERT( input_type && input_name );

        creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_render_graph_resource_input_creation );
        creation.type = crude_gfx_render_graph_resource_string_to_type( cJSON_GetStringValue( input_type ) );
        creation.resource_info.external = false;
        creation.name = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s", cJSON_GetStringValue( input_name ) );
        CRUDE_ARRAY_PUSH( node_creation.inputs, creation );
      }
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

      output_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_render_graph_resource_output_creation );
      output_creation.type = crude_gfx_render_graph_resource_string_to_type( cJSON_GetStringValue( output_type ) );
      output_creation.name = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s", cJSON_GetStringValue( output_name ) );
      output_creation.resource_info.texture.handle.index = CRUDE_RESOURCE_INDEX_INVALID;

      switch ( output_creation.type )
      {
        case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE:
        {
          break;
        }
        case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT:
        {
          cJSON const                                     *output_format;
          cJSON const                                     *output_load_op;
          cJSON const                                     *output_scale;
          
          output_format = cJSON_GetObjectItemCaseSensitive( pass_output, "format" );
          output_load_op = cJSON_GetObjectItemCaseSensitive( pass_output, "op" );
          output_scale = cJSON_GetObjectItemCaseSensitive( pass_output, "scale" );
          CRUDE_ASSERT( output_format && output_load_op && output_scale );
          CRUDE_ASSERT( cJSON_GetArraySize( output_scale ) == 2 );

          output_creation.resource_info.texture.format = crude_gfx_string_to_vk_format( cJSON_GetStringValue( output_format ) );
          output_creation.resource_info.texture.load_op = crude_gfx_string_to_render_pass_operation( cJSON_GetStringValue( output_load_op ) );
          output_creation.resource_info.texture.scale.x = cJSON_GetNumberValue( cJSON_GetArrayItem( output_scale, 0 ) );
          output_creation.resource_info.texture.scale.y = cJSON_GetNumberValue( cJSON_GetArrayItem( output_scale, 1 ) );
          output_creation.resource_info.texture.depth = 1;

          if ( crude_gfx_has_depth( output_creation.resource_info.texture.format ) )
          {
            cJSON const                                   *output_clear_depth;
            cJSON const                                   *output_clear_stencil;

            output_clear_depth = cJSON_GetObjectItemCaseSensitive( pass_output, "clear_depth" );
            output_clear_stencil = cJSON_GetObjectItemCaseSensitive( pass_output, "clear_stencil" );
            output_creation.resource_info.texture.clear_values[ 0 ] = output_clear_depth ? cJSON_GetNumberValue( output_clear_depth ) : 1.f;
            output_creation.resource_info.texture.clear_values[ 1 ] = output_clear_stencil ? cJSON_GetNumberValue( output_clear_stencil ) : 0.f;
          }
          else
          {
            cJSON const                                   *output_clear_color;
            output_clear_color = cJSON_GetObjectItemCaseSensitive( pass_output, "clear_color" );
            if ( output_clear_color )
            {
              for ( uint32 c = 0; c < cJSON_GetArraySize( output_clear_color ); ++c )
              {
                output_creation.resource_info.texture.clear_values[ c ] = cJSON_GetNumberValue( cJSON_GetArrayItem( output_clear_color, c ) );
              }
            }
            else
            {
              if ( output_creation.resource_info.texture.load_op == CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR )
              {
                CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Error parsing output texture %s: load operation is clear, but clear color not specified. Defaulting to 0,0,0,0.\n", output_creation.name );
              }
              output_creation.resource_info.texture.clear_values[ 0 ] = 0.0f;
              output_creation.resource_info.texture.clear_values[ 1 ] = 0.0f;
              output_creation.resource_info.texture.clear_values[ 2 ] = 0.0f;
              output_creation.resource_info.texture.clear_values[ 3 ] = 0.0f;
            }
          }
          break;
        }
        case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_MARKER:
        {
          // :D
          break;
        }
      }
        
      CRUDE_ARRAY_PUSH( node_creation.outputs, output_creation );
    }
    pass_name = cJSON_GetObjectItemCaseSensitive( pass, "name" );
    CRUDE_ASSERT( pass_name );

    pass_enabled = cJSON_GetObjectItemCaseSensitive( pass, "enabled" );
    pass_pipeline_type = cJSON_GetObjectItemCaseSensitive( pass, "type" );

    node_creation.name = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s", cJSON_GetStringValue( pass_name ) );
    node_creation.enabled = pass_enabled ? cJSON_GetNumberValue( pass_enabled ) : 1;
    
    node_creation.type = CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_GRAPHICS;
    if ( pass_pipeline_type )
    {
      if ( crude_string_cmp( cJSON_GetStringValue( pass_pipeline_type ), "compute" ) == 0 )
      {
        node_creation.type = CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_COMPUTE;
      }
      else if ( crude_string_cmp( cJSON_GetStringValue( pass_pipeline_type ), "ray_tracing" ) == 0 )
      {
        node_creation.type = CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_RAY_TRACING;
      }
    }

    crude_gfx_render_graph_node_handle node_handle = crude_gfx_render_graph_builder_create_node( render_graph->builder, &node_creation );
    CRUDE_ARRAY_PUSH( render_graph->nodes, node_handle );
    
    crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_maker );
  }
  
  cJSON_Delete( render_graph_json );
}

void
crude_gfx_render_graph_compile
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Compile render graph \"%i\"", render_graph->name );

  for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++node_index )
  {
    crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ node_index ] );
    CRUDE_ARRAY_SET_LENGTH( node->edges, 0 );
  }

  /* Compute edges */
  for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++node_index )
  {
    crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ node_index ] );
    if ( !node->enabled )
    {
      continue;
    }
    
    for ( uint32 input_index = 0; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
    {
      crude_gfx_render_graph_resource                     *resource;
      crude_gfx_render_graph_resource                     *output_resource;
      crude_gfx_render_graph_node                         *parent_node;
      
      resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->inputs[ input_index ] );
      output_resource = crude_gfx_render_graph_builder_access_resource_by_name( render_graph->builder, resource->name );
      if ( output_resource == NULL && !resource->resource_info.external )
      {
        CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Requested resource is not produced by any node and is not external." );
        continue;
      }

      resource->producer = output_resource->producer;
      resource->resource_info = output_resource->resource_info;
      resource->output_handle = output_resource->output_handle;
      
      parent_node = crude_gfx_render_graph_builder_access_node( render_graph->builder, resource->producer );
      CRUDE_ARRAY_PUSH( parent_node->edges, render_graph->nodes[ node_index ] );
    }
  }

  /* Topological sorting */
  {
    crude_gfx_render_graph_node_handle                    *sorted_nodes;
    crude_gfx_render_graph_node_handle                    *stack;
    uint8                                                 *visited;
    uint32                                                 temporary_allocator_marker;

    temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );

    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( sorted_nodes, CRUDE_ARRAY_LENGTH( render_graph->nodes ), crude_stack_allocator_pack( temporary_allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( visited, CRUDE_ARRAY_LENGTH( render_graph->nodes ), crude_stack_allocator_pack( temporary_allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( stack, CRUDE_ARRAY_LENGTH( render_graph->nodes ), crude_stack_allocator_pack( temporary_allocator ) );

    memset( visited, 0, sizeof( uint8 ) * CRUDE_ARRAY_LENGTH( visited ) );
    
    for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++node_index )
    {
      crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ node_index ] );
      if ( !node->enabled )
      {
        continue;
      }
      
      CRUDE_ARRAY_PUSH( stack, render_graph->nodes[ node_index ] );
      
      while ( CRUDE_ARRAY_LENGTH( stack ) > 0 )
      {
        crude_gfx_render_graph_node_handle                 node_handle;
        crude_gfx_render_graph_node                       *node;
        
        node_handle = CRUDE_ARRAY_BACK( stack );
        if ( visited[ node_handle.index ] == 2 )
        {
          CRUDE_ARRAY_POP( stack );
          continue;
        }
        
        if ( visited[ node_handle.index ] == 1 )
        {
          visited[ node_handle.index ] = 2;
          CRUDE_ARRAY_PUSH( sorted_nodes, node_handle );
          CRUDE_ARRAY_POP( stack );
          continue;
        }
        
        visited[ node_handle.index ] = 1;
        
        node = crude_gfx_render_graph_builder_access_node( render_graph->builder, node_handle );

        if ( CRUDE_ARRAY_LENGTH( node->edges ) == 0 )
        {
          continue;
        }
        
        for ( uint32 edge_index = 0; edge_index < CRUDE_ARRAY_LENGTH( node->edges ); ++edge_index )
        {
          crude_gfx_render_graph_node_handle child_handle = node->edges[ edge_index ];
          if ( !visited[ child_handle.index ] )
          {
            CRUDE_ARRAY_PUSH( stack, child_handle );
          }
        }
      }
    }
    
    CRUDE_ASSERT( CRUDE_ARRAY_LENGTH( sorted_nodes ) == CRUDE_ARRAY_LENGTH( render_graph->nodes ) );
    CRUDE_ARRAY_SET_LENGTH( render_graph->nodes, 0 );
    
    for ( int32 i = CRUDE_ARRAY_LENGTH( sorted_nodes ) - 1; i >= 0; --i )
    {
      CRUDE_ARRAY_PUSH( render_graph->nodes, sorted_nodes[ i ] );
    }
    

    crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
  }
  
  /* Compute Resource Aliasing */
  {
    crude_gfx_render_graph_node_handle                    *allocations;
    crude_gfx_render_graph_node_handle                    *deallocations;
    crude_gfx_texture_handle                              *free_list;
    sizet                                                  resource_count, peak_memory, instant_memory;
    uint32                                                 temporary_allocator_marker;

    temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );

    resource_count = render_graph->builder->resource_cache.resources.used_indices;
    
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( allocations, resource_count, crude_stack_allocator_pack( temporary_allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( deallocations, resource_count, crude_stack_allocator_pack( temporary_allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( free_list, resource_count, crude_stack_allocator_pack( temporary_allocator ) );

    for ( uint32 i = 0; i < resource_count; ++i)
    {
      allocations[ i ] = CRUDE_GFX_RENDER_GRAPH_NODE_HANDLE_INVALID;
    }
    
    for ( uint32 i = 0; i < resource_count; ++i)
    {
      deallocations[ i ] = CRUDE_GFX_RENDER_GRAPH_NODE_HANDLE_INVALID;
    }
    
    
    peak_memory = 0;
    instant_memory = 0;
    for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++node_index )
    {
      crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ node_index ] );
      if ( !node->enabled )
      {
        continue;
      }
      
      for ( uint32 input_index = 0; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
      {
        crude_gfx_render_graph_resource *input_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->inputs[ input_index ] );
        crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, input_resource->output_handle );
        resource->ref_count++;
      }
    }
    
    for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++node_index )
    {
      crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ node_index ] );
      if ( !node->enabled )
      {
        continue;
      }

      for ( uint32 output_index = 0; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        crude_gfx_render_graph_resource                   *output_resource;
        uint32                                             output_resource_index;

        output_resource_index = node->outputs[ output_index ].index;
        output_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->outputs[ output_index ] );
        
        if ( !output_resource->resource_info.external && CRUDE_RESOURCE_HANDLE_IS_INVALID( allocations[ output_resource_index ] ) )
        {
          CRUDE_ASSERT( CRUDE_RESOURCE_HANDLE_IS_INVALID( deallocations[ output_resource_index ] ) );
          allocations[ output_resource_index ] = render_graph->nodes[ node_index ];
          
          if ( output_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
          {
            crude_gfx_render_graph_resource_info          *output_resource_info;
            crude_gfx_texture_creation                     output_resource_texture_creation;
            
            output_resource_info = &output_resource->resource_info;
           
            output_resource_texture_creation = crude_gfx_texture_creation_empty();
            output_resource_texture_creation.name = output_resource->name;
            output_resource_texture_creation.format = output_resource_info->texture.format;
            output_resource_texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
            output_resource_texture_creation.width = render_graph->builder->gpu->renderer_size.x * output_resource_info->texture.scale.x;
            output_resource_texture_creation.height = render_graph->builder->gpu->renderer_size.y * output_resource_info->texture.scale.y;
            output_resource_texture_creation.depth = output_resource_info->texture.depth;
            output_resource_texture_creation.flags = ( node->type != CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_GRAPHICS ) ? ( CRUDE_GFX_TEXTURE_MASK_COMPUTE | CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET ) : CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET;
            output_resource_texture_creation.alias = ( CRUDE_ARRAY_LENGTH( free_list ) > 0 ) ? CRUDE_ARRAY_POP( free_list ) : CRUDE_GFX_TEXTURE_HANDLE_INVALID;

            output_resource_info->texture.handle = crude_gfx_create_texture( render_graph->builder->gpu, &output_resource_texture_creation );
          }

          CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Output %s allocated on node %d\n", output_resource->name, render_graph->nodes[ node_index ].index  );
        }
      }
      
      for ( uint32 input_index = 0; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
      {
        crude_gfx_render_graph_resource                   *input_resource;
        uint32                                             input_resource_index;
        crude_gfx_render_graph_resource                   *input_resource_output_resource;


        input_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->inputs[ input_index ] );
        input_resource_index = input_resource->output_handle.index;

        input_resource_output_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, input_resource->output_handle );
        input_resource_output_resource->ref_count--;
        
        if ( !input_resource_output_resource->resource_info.external && input_resource_output_resource->ref_count == 0 )
        {
          CRUDE_ASSERT( CRUDE_RESOURCE_HANDLE_IS_INVALID( deallocations[ input_resource_index ] ) );
          deallocations[ input_resource_index ] = render_graph->nodes[ node_index ];
          
          if ( input_resource_output_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT || input_resource_output_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE )
          {
            CRUDE_ARRAY_PUSH( free_list, input_resource_output_resource->resource_info.texture.handle );
          }
          
          CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Output %s deallocated on node\n", input_resource_output_resource->name, render_graph->nodes[ node_index ].index );
        }
      }
    }
    
    crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
  }
  
  for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++node_index )
  {
    crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ node_index ] );
    if ( !node->enabled )
    {
      continue;
    }
    
    /* Create render pass */
    if ( ( node->type != CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_COMPUTE ) && CRUDE_RESOURCE_HANDLE_IS_INVALID( node->render_pass ) )
    {
      crude_gfx_render_pass_creation render_pass_creation = crude_gfx_render_pass_creation_empty();
      render_pass_creation.name = node->name;
      
      for ( uint32 output_index = 0; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        crude_gfx_render_graph_resource *output_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->outputs[ output_index ] );
        crude_gfx_render_graph_resource_info *info = &output_resource->resource_info;

        if ( output_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
        {
          if ( crude_gfx_has_depth_or_stencil( info->texture.format ) )
          {
            render_pass_creation.depth_stencil_format = info->texture.format;
            render_pass_creation.stencil_operation = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
            render_pass_creation.depth_operation = CRUDE_GFX_RENDER_PASS_OPERATION_CLEAR;
          }
          else
          {
            render_pass_creation.color_formats[ render_pass_creation.num_render_targets ] = info->texture.format;
            render_pass_creation.color_operations[ render_pass_creation.num_render_targets ] = info->texture.load_op;
            render_pass_creation.color_final_layouts[ render_pass_creation.num_render_targets ] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ++render_pass_creation.num_render_targets;
          }
        }
      }

      for ( uint32 input_index = 0; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
      {
        crude_gfx_render_graph_resource *input_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->inputs[ input_index ] );
        crude_gfx_render_graph_resource_info *info = &input_resource->resource_info;

        if ( input_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
        {
          if ( crude_gfx_has_depth_or_stencil( info->texture.format ) )
          {
            render_pass_creation.depth_stencil_format = info->texture.format;
            render_pass_creation.stencil_operation = CRUDE_GFX_RENDER_PASS_OPERATION_DONT_CARE;
            render_pass_creation.depth_operation = CRUDE_GFX_RENDER_PASS_OPERATION_LOAD;
          }
          else
          {
            render_pass_creation.color_formats[ render_pass_creation.num_render_targets ] = info->texture.format;
            render_pass_creation.color_operations[ render_pass_creation.num_render_targets ] = CRUDE_GFX_RENDER_PASS_OPERATION_LOAD;
            render_pass_creation.color_final_layouts[ render_pass_creation.num_render_targets ] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ++render_pass_creation.num_render_targets;
          }
        }
      }
      
      node->render_pass =  crude_gfx_create_render_pass( render_graph->builder->gpu, &render_pass_creation );
    }
    
    /* Create framebuffer */
    if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( node->framebuffer ) )
    {
      crude_gfx_framebuffer_creation                       framebuffer_creation;
      uint32                                               width, height;
      
      width = height = 0;

      framebuffer_creation = crude_gfx_framebuffer_creation_empty();
      framebuffer_creation.name = crude_string_buffer_append_use_f( &render_graph->builder->gpu->objects_names_string_buffer, "%s", node->name );

      for ( uint32 output_index = 0; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        crude_gfx_render_graph_resource *output_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->outputs[ output_index ] );
        crude_gfx_render_graph_resource_info *output_resource_info = &output_resource->resource_info;

        if ( output_resource->type != CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
        {
          continue;
        }

        if ( width == 0 )
        {
          width = render_graph->builder->gpu->renderer_size.x * output_resource_info->texture.scale.x;
        }
        else
        {
          CRUDE_ASSERT( width == render_graph->builder->gpu->renderer_size.x * output_resource_info->texture.scale.x );
        }

        if ( height == 0 )
        {
          height = render_graph->builder->gpu->renderer_size.y * output_resource_info->texture.scale.y;
        }
        else
        {
          CRUDE_ASSERT( height == render_graph->builder->gpu->renderer_size.y * output_resource_info->texture.scale.y );
        }

        if ( crude_gfx_has_depth_or_stencil( output_resource_info->texture.format ) )
        {
          framebuffer_creation.depth_stencil_texture = output_resource_info->texture.handle;
        }
        else
        {
          framebuffer_creation.output_textures[ framebuffer_creation.num_render_targets++ ] = output_resource_info->texture.handle;
        }
      }

      for ( uint32 input_index = 0; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
      {
        crude_gfx_render_graph_resource *input_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->inputs[ input_index ] );

        if ( input_resource->type != CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
        {
          continue;
        }
          
        crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource_by_name( render_graph->builder, input_resource->name );
        crude_gfx_render_graph_resource_info *info = &resource->resource_info;

        input_resource->resource_info.texture.handle = info->texture.handle;

        if ( width == 0 )
        {
          width = render_graph->builder->gpu->renderer_size.x * info->texture.scale.x;
        }
        else
        {
          CRUDE_ASSERT( width == render_graph->builder->gpu->renderer_size.x * info->texture.scale.x );
        }

        if ( height == 0 )
        {
          height = render_graph->builder->gpu->renderer_size.y * info->texture.scale.y;
        }
        else
        {
          CRUDE_ASSERT( height == render_graph->builder->gpu->renderer_size.y * info->texture.scale.y );
        }

        if ( input_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE )
        {
          continue;
        }

        if ( crude_gfx_has_depth_or_stencil( info->texture.format ) )
        {
          framebuffer_creation.depth_stencil_texture = info->texture.handle;
        }
        else
        {
          framebuffer_creation.output_textures[ framebuffer_creation.num_render_targets++ ] = info->texture.handle;
        }
      }
      
      framebuffer_creation.width = width;
      framebuffer_creation.height = height;

      framebuffer_creation.manual_resources_free = true;
      node->framebuffer = crude_gfx_create_framebuffer( render_graph->builder->gpu, &framebuffer_creation );
    }
  }
}

void
crude_gfx_render_graph_render
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_gfx_cmd_buffer                               *gpu_commands
)
{
  for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++node_index )
  {
    crude_gfx_render_graph_node                           *node;

    node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ node_index ] );
    
    if ( !node->enabled )
    {
      continue;
    }
    
    crude_gfx_cmd_push_marker( gpu_commands, node->name );
    
    if ( node->type == CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_GRAPHICS )
    {
      uint32                                               width, height;
      crude_gfx_rect2d_int                                 scissor;
      crude_gfx_viewport                                   dev_viewport;

      width = height = 0;

      for ( uint32 input_index = 0; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
      {
        crude_gfx_render_graph_resource *input_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->inputs[ input_index ] );
        crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, input_resource->output_handle );

        if ( !resource || resource->resource_info.external )
        {
          continue;
        }

        if ( input_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE )
        {
          crude_gfx_texture *texture = crude_gfx_access_texture( gpu_commands->gpu, resource->resource_info.texture.handle );
          crude_gfx_cmd_add_image_barrier( gpu_commands, texture, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1, crude_gfx_has_depth_or_stencil( resource->resource_info.texture.format ) );
        }
        else if ( input_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
        {
          crude_gfx_texture *texture = crude_gfx_access_texture( gpu_commands->gpu, resource->resource_info.texture.handle );
          width = texture->width;
          height = texture->height;
          
          if ( crude_gfx_has_depth_or_stencil( texture->vk_format ) )
          {
            crude_gfx_cmd_add_image_barrier( gpu_commands, texture, CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE, 0, 1, true );
          }
          else
          {
            crude_gfx_cmd_add_image_barrier( gpu_commands, texture, CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET, 0, 1, false );
          }
        }
      }
      
      for ( uint32 output_index = 0; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->outputs[ output_index ] );
        
        if ( resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
        {
          crude_gfx_texture                               *texture;

          texture = crude_gfx_access_texture( gpu_commands->gpu, resource->resource_info.texture.handle );
          width = texture->width;
          height = texture->height;

          if ( crude_gfx_has_depth_or_stencil( texture->vk_format ) )
          {
            float32 *clear_values = &resource->resource_info.texture.clear_values[ 0 ];
            crude_gfx_cmd_set_clear_depth_and_stencil( gpu_commands, clear_values[ 0 ], clear_values[ 1 ] );
            crude_gfx_cmd_add_image_barrier( gpu_commands, texture, CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE, 0, 1, true );
          }
          else
          {
            float32 *clear_values = &resource->resource_info.texture.clear_values[ 0 ];
            crude_gfx_cmd_set_clear_color_f32( gpu_commands, clear_values[ 0 ], clear_values[ 1 ], clear_values[ 2 ], clear_values[ 3 ], output_index );
            crude_gfx_cmd_add_image_barrier( gpu_commands, texture, CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET, 0, 1, false );
          }
        }
      }
      
      scissor = CRUDE_COMPOUNT_EMPTY( crude_gfx_rect2d_int );
      scissor.x = 0; 
      scissor.y = 0;
      scissor.width = CRUDE_STATIC_CAST( uint16, width );
      scissor.height = CRUDE_STATIC_CAST( uint16, height );
      crude_gfx_cmd_set_scissor( gpu_commands, &scissor );

      dev_viewport = CRUDE_COMPOUNT_EMPTY( crude_gfx_viewport );
      dev_viewport.rect.x = 0;
      dev_viewport.rect.y = 0;
      dev_viewport.rect.width = CRUDE_STATIC_CAST( uint16, width );
      dev_viewport.rect.height = CRUDE_STATIC_CAST( uint16, height );
      dev_viewport.min_depth = 0.0f;
      dev_viewport.max_depth = 1.0f;
      crude_gfx_cmd_set_viewport( gpu_commands, &dev_viewport );
      
      crude_gfx_render_graph_render_pass_container_pre_render( node->render_graph_pass_container, gpu_commands );
      crude_gfx_cmd_bind_render_pass( gpu_commands, node->render_pass, node->framebuffer, false );
      crude_gfx_render_graph_render_pass_container_render( node->render_graph_pass_container, gpu_commands );
      crude_gfx_cmd_end_render_pass( gpu_commands );
      crude_gfx_render_graph_render_pass_container_post_render( node->render_graph_pass_container, gpu_commands );
    }
    else if ( node->type == CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_COMPUTE )
    {
      for ( uint32 input_index = 0; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
      {
        crude_gfx_render_graph_resource *input_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->inputs[ input_index ] );
        crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, input_resource->output_handle );
        
        if ( !resource || resource->resource_info.external )
        {
          continue;
        }

        if ( input_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE )
        {
          crude_gfx_texture *texture = crude_gfx_access_texture( gpu_commands->gpu, resource->resource_info.texture.handle );
          crude_gfx_cmd_add_image_barrier( gpu_commands, texture, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1, crude_gfx_has_depth( resource->resource_info.texture.format ) );
        }
      }
      
      for ( uint32 output_index = 0; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->outputs[ output_index ] );
        
        if ( resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
        {
          crude_gfx_texture *texture = crude_gfx_access_texture( gpu_commands->gpu, resource->resource_info.texture.handle );
          crude_gfx_cmd_add_image_barrier( gpu_commands, texture, CRUDE_GFX_RESOURCE_STATE_UNORDERED_ACCESS, 0, 1, false );
        }
      }
      
      crude_gfx_render_graph_render_pass_container_pre_render( node->render_graph_pass_container, gpu_commands );
      crude_gfx_render_graph_render_pass_container_render( node->render_graph_pass_container, gpu_commands );
      crude_gfx_render_graph_render_pass_container_post_render( node->render_graph_pass_container, gpu_commands );
    }
    else if ( node->type == CRUDE_GFX_RENDER_GRAPH_NODE_TYPE_RAY_TRACING )
    {
      crude_gfx_render_graph_render_pass_container_pre_render( node->render_graph_pass_container, gpu_commands );
      crude_gfx_render_graph_render_pass_container_render( node->render_graph_pass_container, gpu_commands );
      crude_gfx_render_graph_render_pass_container_post_render( node->render_graph_pass_container, gpu_commands );
    }

    crude_gfx_cmd_pop_marker( gpu_commands );
  }
}

void
crude_gfx_render_graph_on_resize
(
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_render_graph_on_resize" );
  for ( size_t i = 0; i < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++i )
  {
    crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ i ] );
    if ( !node->enabled )
    {
      continue;
    }
    
    crude_gfx_resize_framebuffer( render_graph->builder->gpu, node->framebuffer, new_width, new_height );
    crude_gfx_render_graph_render_pass_container_on_resize( node->render_graph_pass_container, new_width, new_height );
  }
  CRUDE_PROFILER_ZONE_END;
}

void
crude_gfx_render_graph_on_techniques_reloaded
(
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++i )
  {
    crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ i ] );
    crude_gfx_render_graph_render_pass_container_on_techniques_reloaded( node->render_graph_pass_container );
  }
}

/************************************************
 *
 * Render Graph Builder Function Implementation
 * 
 ***********************************************/
void
crude_gfx_render_graph_builder_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_device                                   *gpu
)
{
  builder->allocator_container = gpu->allocator_container;
  builder->gpu = gpu;
  
  crude_gfx_render_graph_builder_resource_cache_initialize( builder );
  crude_gfx_render_graph_builder_node_cache_initialize( builder );
  crude_gfx_render_graph_builder_render_pass_cache_initialize( builder );
}

void
crude_gfx_render_graph_builder_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  crude_gfx_render_graph_builder_resource_cache_deinitialize( builder );
  crude_gfx_render_graph_builder_node_cache_deinitialize( builder );
  crude_gfx_render_graph_builder_render_pass_cache_deinitialize( builder );
}

void
crude_gfx_render_graph_builder_register_render_pass
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ char const                                         *name,
  _In_ crude_gfx_render_graph_pass_container               render_pass
)
{
  uint64 key = crude_hash_string( name, 0 );
  int64 handle_index = CRUDE_HASHMAP_GET_INDEX( builder->render_pass_cache.render_pass_map, key );
  if ( handle_index >= 0 )
  {
    CRUDE_ASSERT( false );
    return;
  }

  handle_index = CRUDE_HASHMAP_GET_INDEX( builder->node_cache.node_map, key );
  if ( handle_index < 0 )
  {
    CRUDE_ASSERT( false );
    return;
  }
  
  CRUDE_HASHMAP_SET( builder->render_pass_cache.render_pass_map, key, render_pass );

  crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( builder, builder->node_cache.node_map[ handle_index ].value );
  node->render_graph_pass_container = render_pass;
}

void
crude_gfx_render_graph_builder_unregister_render_pass
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ char const                                         *name
)
{
  uint64 key = crude_hash_string( name, 0 );
  CRUDE_HASHMAP_REMOVE( builder->render_pass_cache.render_pass_map, key );
}

void
crude_gfx_render_graph_builder_unregister_all_render_passes
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  for ( uint32 i = 0; i < CRUDE_HASHMAP_CAPACITY( builder->render_pass_cache.render_pass_map ); ++i )
  {
    if ( crude_hashmap_backet_key_valid( builder->render_pass_cache.render_pass_map[ i ].key ) )
    {
      builder->render_pass_cache.render_pass_map[ i ].key = CRUDE_HASHMAP_BACKET_STATE_EMPTY;
    }
  }
}

crude_gfx_render_graph_node_handle
crude_gfx_render_graph_builder_create_node
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_render_graph_node_creation const         *creation
)
{
  crude_gfx_render_graph_node                             *node;
  crude_gfx_render_graph_node_handle                       node_handle;
  
  node_handle = crude_gfx_render_graph_builder_obtain_node( builder );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( node_handle ) )
  {
    return node_handle;
  }

  node = crude_gfx_render_graph_builder_access_node( builder, node_handle );
  crude_string_copy( node->name, creation->name, sizeof( node->name ) );
  node->enabled = creation->enabled;
  node->type = creation->type;
  node->framebuffer = CRUDE_GFX_FRAMEBUFFER_HANDLE_INVALID;
  node->render_pass = CRUDE_GFX_RENDER_PASS_HANDLE_INVALID;
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->inputs, CRUDE_ARRAY_LENGTH( creation->inputs ), builder->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->outputs, CRUDE_ARRAY_LENGTH( creation->outputs ), builder->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->edges, CRUDE_ARRAY_LENGTH( creation->outputs ), builder->allocator_container );
  
  {
    uint64 key = crude_hash_string( node->name, 0 );
    CRUDE_HASHMAP_SET( builder->node_cache.node_map, key, node_handle );
  }

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( node->outputs ); ++i )
  {
    node->outputs[ i ] = crude_gfx_render_graph_builder_create_node_output( builder, &creation->outputs[ i ], node_handle );
  }
  
  for ( sizet i = 0; i < CRUDE_ARRAY_LENGTH( node->inputs ); ++i )
  {
    node->inputs[ i ] = crude_gfx_render_graph_builder_create_node_input( builder, &creation->inputs[ i ] );
  }
  
  return node_handle;
}

crude_gfx_render_graph_resource_handle
crude_gfx_render_graph_builder_create_node_output
(
  _In_ crude_gfx_render_graph_builder                        *builder,
  _In_ crude_gfx_render_graph_resource_output_creation const *creation,
  _In_ crude_gfx_render_graph_node_handle                     producer
)
{
  crude_gfx_render_graph_resource_handle resource_handle = crude_gfx_render_graph_builder_obtain_resource( builder );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( resource_handle ) )
  {
    return resource_handle;
  }

  crude_gfx_render_graph_resource* resource = crude_gfx_render_graph_builder_access_resource( builder, resource_handle );
  resource->type = creation->type;
  
  crude_string_copy( resource->name, creation->name, sizeof( resource->name ) );
  
  if ( creation->type != CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_REFERENCE )
  {
    resource->resource_info.texture.handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;

    resource->resource_info = creation->resource_info;
    resource->output_handle = resource_handle;
    resource->producer = producer;
    resource->ref_count = 0;

    uint64 key = crude_hash_string( resource->name, 0 );
    CRUDE_HASHMAP_SET( builder->resource_cache.resource_map, key, resource_handle );
  }

  return resource_handle;
}

crude_gfx_render_graph_resource_handle
crude_gfx_render_graph_builder_create_node_input
(
  _In_ crude_gfx_render_graph_builder                       *builder,
  _In_ crude_gfx_render_graph_resource_input_creation const *creation
)
{
  crude_gfx_render_graph_resource_handle resource_handle = crude_gfx_render_graph_builder_obtain_resource( builder );
  if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( resource_handle ) )
  {
    return resource_handle;
  }
  
  crude_gfx_render_graph_resource* resource = crude_gfx_render_graph_builder_access_resource( builder, resource_handle );
  resource->resource_info = CRUDE_COMPOUNT_EMPTY( crude_gfx_render_graph_resource_info );
  resource->producer = CRUDE_GFX_RENDER_GRAPH_NODE_HANDLE_INVALID;
  resource->output_handle = CRUDE_GFX_RENDER_GRAPH_RESOURCE_HANDLE_INVALID;
  resource->type = creation->type;
  resource->ref_count = 0;

  crude_string_copy( resource->name, creation->name, sizeof( resource->name ) );
  
  return resource_handle;
}

crude_gfx_render_graph_node*
crude_gfx_render_graph_builder_access_node
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_render_graph_node_handle                  handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_render_graph_node*, crude_resource_pool_access_resource( &builder->node_cache.nodes, handle.index ) );
}

crude_gfx_render_graph_node*
crude_gfx_render_graph_builder_access_node_by_name
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ char const                                         *name
)
{
  uint64 key = crude_hash_string( name, 0 );
  int64 handle_index = CRUDE_HASHMAP_GET_INDEX( builder->node_cache.node_map, key );
  if ( handle_index < 0 )
  {
    return NULL;
  }
  return CRUDE_REINTERPRET_CAST( crude_gfx_render_graph_node*, crude_resource_pool_access_resource( &builder->node_cache.nodes, builder->node_cache.node_map[ handle_index ].value.index ) );

}

crude_gfx_render_graph_node_handle
crude_gfx_render_graph_builder_obtain_node
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  return CRUDE_COMPOUNT( crude_gfx_render_graph_node_handle, { crude_resource_pool_obtain_resource( &builder->node_cache.nodes ) } );
}


crude_gfx_render_graph_resource_handle
crude_gfx_render_graph_builder_obtain_resource
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  return CRUDE_COMPOUNT( crude_gfx_render_graph_resource_handle, { crude_resource_pool_obtain_resource( &builder->resource_cache.resources ) } );
}

crude_gfx_render_graph_resource*
crude_gfx_render_graph_builder_access_resource
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_render_graph_resource_handle              handle
)
{
  return CRUDE_REINTERPRET_CAST( crude_gfx_render_graph_resource*, crude_resource_pool_access_resource( &builder->resource_cache.resources, handle.index ) );
}

crude_gfx_render_graph_resource*
crude_gfx_render_graph_builder_access_resource_by_name
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ char const                                         *name
)
{
  uint64 key = crude_hash_string( name, 0 );
  int64 handle_index = CRUDE_HASHMAP_GET_INDEX( builder->resource_cache.resource_map, key );
  if ( handle_index < 0 )
  {
    CRUDE_ASSERT( false );
    return NULL;
  }
  
  return CRUDE_REINTERPRET_CAST( crude_gfx_render_graph_resource*, crude_resource_pool_access_resource( &builder->resource_cache.resources, builder->resource_cache.resource_map[ handle_index ].value.index ) );
}

void
crude_gfx_render_graph_builder_resource_cache_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  crude_resource_pool_initialize( &builder->resource_cache.resources, builder->allocator_container, CRUDE_GRAPHICS_RENDER_GRAPH_MAX_RESOURCES_COUNT, sizeof( crude_gfx_render_graph_resource ) );
  builder->resource_cache.resource_map = NULL;
  CRUDE_HASHMAP_INITIALIZE( builder->resource_cache.resource_map, builder->allocator_container );
}

void
crude_gfx_render_graph_builder_node_cache_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  crude_resource_pool_initialize( &builder->node_cache.nodes, builder->allocator_container, CRUDE_GRAPHICS_RENDER_GRAPH_MAX_NODES_COUNT, sizeof( crude_gfx_render_graph_node ) );
  builder->node_cache.node_map = NULL;
  CRUDE_HASHMAP_INITIALIZE( builder->node_cache.node_map, builder->allocator_container );
}

void
crude_gfx_render_graph_builder_render_pass_cache_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  CRUDE_HASHMAP_INITIALIZE( builder->render_pass_cache.render_pass_map, builder->allocator_container );
}

void
crude_gfx_render_graph_builder_resource_cache_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  for ( int32 i = 0; i < CRUDE_HASHMAP_CAPACITY( builder->resource_cache.resource_map ); ++i )
  {
    if ( !crude_hashmap_backet_key_valid( builder->resource_cache.resource_map[ i ].key ) )
    {
      continue;
    }

    crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( builder, builder->resource_cache.resource_map[ i ].value );

    bool is_texture_type = ( resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE || resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT );
    if ( is_texture_type && CRUDE_RESOURCE_HANDLE_IS_VALID( resource->resource_info.texture.handle ) )
    {
      crude_gfx_texture *texture = crude_gfx_access_texture( builder->gpu, resource->resource_info.texture.handle );
      crude_gfx_destroy_texture( builder->gpu, texture->handle );
    }
  }

  crude_resource_pool_free_all_resource( &builder->resource_cache.resources );
  crude_resource_pool_deinitialize( &builder->resource_cache.resources );
  CRUDE_HASHMAP_DEINITIALIZE( builder->resource_cache.resource_map );
}

void
crude_gfx_render_graph_builder_node_cache_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  crude_resource_pool_free_all_resource( &builder->node_cache.nodes );
  crude_resource_pool_deinitialize( &builder->node_cache.nodes );
  CRUDE_HASHMAP_DEINITIALIZE( builder->node_cache.node_map );
}

void
crude_gfx_render_graph_builder_render_pass_cache_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  CRUDE_HASHMAP_DEINITIALIZE( builder->render_pass_cache.render_pass_map );
}

/************************************************
 *
 * Render Graph Pass Container Utils
 * 
 ***********************************************/
void
crude_gfx_render_graph_render_pass_container_render
(
  _In_ crude_gfx_render_graph_pass_container               container,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  container.render( container.ctx, primary_cmd );
}

void
crude_gfx_render_graph_render_pass_container_pre_render
(
  _In_ crude_gfx_render_graph_pass_container               container,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  container.pre_render( container.ctx, primary_cmd );
}

void
crude_gfx_render_graph_render_pass_container_post_render
(
  _In_ crude_gfx_render_graph_pass_container               container,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  container.post_render( container.ctx, primary_cmd );
}

void
crude_gfx_render_graph_render_pass_container_on_resize
(
  _In_ crude_gfx_render_graph_pass_container               container,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
  container.on_resize( container.ctx, new_width, new_height );
}

void
crude_gfx_render_graph_render_pass_container_on_techniques_reloaded
(
  _In_ crude_gfx_render_graph_pass_container               container
)
{
  container.on_techniques_reloaded( container.ctx );
}

static void
empty_pass_on_resize_
(
  _In_ void                                               *ctx,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

static void
empty_pass_on_techniques_reloaded_
(
  _In_ void                                               *ctx
)
{
}

static void
empty_pass_render_
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *gpu_commands
)
{
}

static void
empty_pass_pre_render_
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *gpu_commands
)
{
}

static void
empty_pass_post_render_
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *gpu_commands
)
{
}

crude_gfx_render_graph_pass_container
crude_gfx_render_graph_pass_container_empty
(
)
{
  crude_gfx_render_graph_pass_container container = CRUDE_COMPOUNT_EMPTY( crude_gfx_render_graph_pass_container );
  container.pre_render = empty_pass_pre_render_;
  container.render = empty_pass_render_;
  container.post_render = empty_pass_post_render_;
  container.on_resize = empty_pass_on_resize_;
  container.on_techniques_reloaded = empty_pass_on_techniques_reloaded_;
  container.ctx = NULL;
  return container;
}

/************************************************
 *
 * Render Graph Resources
 * 
 ***********************************************/
crude_gfx_render_graph_resource_type
crude_gfx_render_graph_resource_string_to_type
(
  _In_ char const                                         *str
)
{
  if ( strcmp( str, "texture" ) == 0 )
  {
    return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE;
  }
  if ( strcmp( str, "attachment" ) == 0 )
  {
    return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT;
  }
  if ( strcmp( str, "marker" ) == 0 )
  {
    return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_MARKER;
  }
  if ( strcmp( str, "reference" ) == 0 )
  {
    return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_REFERENCE;
  }
  
  CRUDE_ABORT( CRUDE_CHANNEL_GRAPHICS, "Can't convert string to resoruce type for render graph" );
  return CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_INVALID;
}

char const*
crude_gfx_render_graph_resource_type_to_string
(
  _In_ crude_gfx_render_graph_resource_type                type
)
{
  switch ( type )
  {
  case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT:    return "attachment";
  case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE:       return "texture";
  case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_MARKER:        return "marker";
  case CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_REFERENCE:     return "reference";
  }
  CRUDE_ASSERT( false );
  return "Invalid";
}