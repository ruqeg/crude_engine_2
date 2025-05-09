#include <cJSON.h>

#include <core/file.h>
#include <core/log.h>
#include <core/assert.h>
#include <core/hash_map.h>
#include <core/string.h>

#include <graphics/render_graph.h>

/************************************************
 *
 * Render Graph Utils Static Functions Declataion
 * 
 ***********************************************/
crude_gfx_render_graph_resource_type
string_to_resource_type_
(
  _In_ char const                                         *input_type
);

crude_gfx_render_pass_operation
string_to_render_pass_operation_
(
  _In_ char const                                         *op
);

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
  crude_linear_allocator_initialize( &render_graph->local_allocator, CRUDE_RMEGA( 1u ), "RenderGraphLocalAllocator" );
  render_graph->local_allocator_container = crude_linear_allocator_pack( &render_graph->local_allocator );
  render_graph->builder = builder;

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( render_graph->nodes, CRUDE_GFX_RENDER_GRAPH_MAX_NODES_COUNT, render_graph->local_allocator_container); 
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
    
    CRUDE_ARRAY_FREE( node->inputs ); 
    CRUDE_ARRAY_FREE( node->outputs ); 
    CRUDE_ARRAY_FREE( node->edges ); 
  }
  
  CRUDE_ARRAY_FREE( render_graph->nodes ); 

  crude_linear_allocator_deinitialize( &render_graph->local_allocator );
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
    CRUDE_ASSERT( pass_inputs && pass_outputs );

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
        .type = string_to_resource_type_( cJSON_GetStringValue( input_type ) ),
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
      output_creation.type = string_to_resource_type_( cJSON_GetStringValue( output_type ) );
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
          output_creation.resource_info.texture.load_op = string_to_render_pass_operation_( cJSON_GetStringValue( output_load_op ) );
          output_creation.resource_info.texture.width = cJSON_GetNumberValue( cJSON_GetArrayItem( output_resolution, 0 ) );
          output_creation.resource_info.texture.height = cJSON_GetNumberValue( cJSON_GetArrayItem( output_resolution, 1 ) );
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
    pass_name = cJSON_GetObjectItemCaseSensitive( pass, "name" );
    pass_enabled = cJSON_GetObjectItemCaseSensitive( pass, "enabled" );
    CRUDE_ASSERT( pass_name );

    node_creation.name = crude_string_buffer_append_use_f( &string_buffer, "%s", cJSON_GetStringValue( pass_name ) );
    node_creation.enabled = pass_enabled ? cJSON_GetNumberValue( pass_enabled ) : 1;
    
    crude_gfx_render_graph_node_handle node_handle = crude_gfx_render_graph_builder_create_node( render_graph->builder, &node_creation );
    CRUDE_ARRAY_PUSH( render_graph->nodes, node_handle );
  }
  
  cJSON_Delete( render_graph_json );
  crude_stack_allocator_free_marker( temp_allocator, temp_allocator_container_maker );
}

void
crude_gfx_render_graph_compile
(
  _In_ crude_gfx_render_graph                             *render_graph
)
{
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

    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( sorted_nodes, CRUDE_ARRAY_LENGTH( render_graph->nodes ), render_graph->local_allocator_container );

    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( visited, CRUDE_ARRAY_LENGTH( render_graph->nodes ), render_graph->local_allocator_container );
    memset( visited, 0, sizeof( uint8 ) * CRUDE_ARRAY_LENGTH( visited ) );
    
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( stack, CRUDE_ARRAY_LENGTH( render_graph->nodes ), render_graph->local_allocator_container );
    
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
    
    // TODO use temproray allocator? (currently linear)
    CRUDE_ARRAY_FREE( visited );
    CRUDE_ARRAY_FREE( stack );
    CRUDE_ARRAY_FREE( sorted_nodes );
  }
  
  /* Compute Resource Aliasing */
  {
    crude_gfx_render_graph_node_handle                    *allocations;
    crude_gfx_render_graph_node_handle                    *deallocations;
    crude_gfx_texture_handle                              *free_list;
    sizet                                                  resource_count, peak_memory, instant_memory;

    resource_count = render_graph->builder->resource_cache.resources.used_indices;
    
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( allocations, resource_count, render_graph->local_allocator_container );
    for ( uint32 i = 0; i < resource_count; ++i)
    {
      allocations[ i ] = CRUDE_GFX_RENDER_GRAPH_NODE_HANDLE_INVALID;
    }
    
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( deallocations, resource_count, render_graph->local_allocator_container );
    for ( uint32 i = 0; i < resource_count; ++i)
    {
      deallocations[ i ] = CRUDE_GFX_RENDER_GRAPH_NODE_HANDLE_INVALID;
    }
    
    CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( free_list, resource_count, render_graph->local_allocator_container );
    
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
            output_resource_texture_creation.width = output_resource_info->texture.width;
            output_resource_texture_creation.height = output_resource_info->texture.height;
            output_resource_texture_creation.depth = output_resource_info->texture.depth;
            output_resource_texture_creation.flags = CRUDE_GFX_TEXTURE_MASK_RENDER_TARGET;
            output_resource_texture_creation.mipmaps = 1;
            output_resource_texture_creation.alias = ( CRUDE_ARRAY_LENGTH( free_list ) > 0 ) ? CRUDE_ARRAY_POP( free_list ) : CRUDE_GFX_TEXTURE_HANDLE_INVALID;

            output_resource_info->texture.texture = crude_gfx_create_texture( render_graph->builder->gpu, &output_resource_texture_creation );
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
            CRUDE_ARRAY_PUSH( free_list, input_resource_output_resource->resource_info.texture.texture );
          }
          
          CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Output %s deallocated on node\n", input_resource_output_resource->name, render_graph->nodes[ node_index ].index );
        }
      }
    }
    
    // TODO use temproray allocator? (currently linear)
    CRUDE_ARRAY_FREE( allocations );
    CRUDE_ARRAY_FREE( deallocations );
    CRUDE_ARRAY_FREE( free_list );
  }
  
  for ( uint32 node_index = 0; node_index < CRUDE_ARRAY_LENGTH( render_graph->nodes ); ++node_index )
  {
    crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ node_index ] );
    if ( !node->enabled )
    {
      continue;
    }
    
    /* Create render pass */
    if ( CRUDE_RESOURCE_HANDLE_IS_INVALID( node->render_pass ) )
    {
      crude_gfx_render_pass_creation render_pass_creation = crude_gfx_render_pass_creation_empty();
      render_pass_creation.name = node->name;
      
      for ( uint32 output_index = 0; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
      {
        crude_gfx_render_graph_resource *output_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->outputs[ output_index ] );
        crude_gfx_render_graph_resource_info *info = &output_resource->resource_info;

        if ( output_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
        {
          if ( info->texture.format == VK_FORMAT_D32_SFLOAT )
          {
            render_pass_creation.depth_stencil_format = info->texture.format;
            render_pass_creation.stencil_operation = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
          if ( info->texture.format == VK_FORMAT_D32_SFLOAT )
          {
            render_pass_creation.depth_stencil_format = info->texture.format;
            render_pass_creation.stencil_operation = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
      framebuffer_creation.render_pass = node->render_pass;
      framebuffer_creation.name = node->name;

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
          width = output_resource_info->texture.width;
        }
        else
        {
          CRUDE_ASSERT( width == output_resource_info->texture.width );
        }

        if ( height == 0 )
        {
          height = output_resource_info->texture.height;
        }
        else
        {
          CRUDE_ASSERT( height == output_resource_info->texture.height );
        }
        
        if ( output_resource_info->texture.format == VK_FORMAT_D32_SFLOAT )
        {
          framebuffer_creation.depth_stencil_texture = output_resource_info->texture.texture;
        }
        else
        {
          framebuffer_creation.output_textures[ framebuffer_creation.num_render_targets++ ] = output_resource_info->texture.texture;
        }
      }

      for ( uint32 input_index = 0; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
      {
        crude_gfx_render_graph_resource *input_resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->inputs[ input_index ] );

        if ( input_resource->type != CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT && input_resource->type != CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_REFERENCE )
        {
          continue;
        }
          
        crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource_by_name( render_graph->builder, input_resource->name );
        crude_gfx_render_graph_resource_info *info = &resource->resource_info;

        input_resource->resource_info.texture.texture = info->texture.texture;

        if ( width == 0 )
        {
          width = info->texture.width;
        }
        else
        {
          CRUDE_ASSERT( width == info->texture.width );
        }

        if ( height == 0 )
        {
          height = info->texture.height;
        }
        else
        {
          CRUDE_ASSERT( width == info->texture.height );
        }

        if ( input_resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE )
        {
          continue;
        }

        if ( info->texture.format == VK_FORMAT_D32_SFLOAT )
        {
          framebuffer_creation.depth_stencil_texture = info->texture.texture;
        }
        else
        {
          framebuffer_creation.output_textures[ framebuffer_creation.num_render_targets++ ] = info->texture.texture;
        }
      }
      
      framebuffer_creation.width = width;
      framebuffer_creation.height = height;
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
    uint32                                                 width, height;

    node = crude_gfx_render_graph_builder_access_node( render_graph->builder, render_graph->nodes[ node_index ] );
    if ( !node->enabled )
    {
      continue;
    }
    
    // TODO add clear to json
    crude_gfx_cmd_set_clear_color( gpu_commands, 0, ( VkClearValue ){ .color = { 0.3f, 0.3f, 0.3f, 1.f } } );
    crude_gfx_cmd_set_clear_color( gpu_commands, 1, ( VkClearValue ){ .depthStencil = { 1.0f, 0 } } );
    
    width = height = 0;

    for ( uint32 input_index = 0; input_index < CRUDE_ARRAY_LENGTH( node->inputs ); ++input_index )
    {
      crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->inputs[ input_index ] );
      
      if ( resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE )
      {
        crude_gfx_texture *texture = crude_gfx_access_texture( gpu_commands->gpu, resource->resource_info.texture.texture );
        crude_gfx_cmd_add_image_barrier( gpu_commands, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET, CRUDE_GFX_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, 1, resource->resource_info.texture.format == VK_FORMAT_D32_SFLOAT );
      }
      else if ( resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
      {
        crude_gfx_texture *texture = crude_gfx_access_texture( gpu_commands->gpu, resource->resource_info.texture.texture );
        width = texture->width;
        height = texture->height;
      }
    }
    
    for ( uint32 output_index = 0; output_index < CRUDE_ARRAY_LENGTH( node->outputs ); ++output_index )
    {
      crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( render_graph->builder, node->outputs[ output_index ] );
      
      if ( resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT )
      {
        crude_gfx_texture *texture = crude_gfx_access_texture( gpu_commands->gpu, resource->resource_info.texture.texture );
        width = texture->width;
        height = texture->height;
        
        if ( texture->vk_format == VK_FORMAT_D32_SFLOAT )
        {
          crude_gfx_cmd_add_image_barrier( gpu_commands, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RESOURCE_STATE_DEPTH_WRITE, 0, 1, resource->resource_info.texture.format == VK_FORMAT_D32_SFLOAT );
        }
        else
        {
          crude_gfx_cmd_add_image_barrier( gpu_commands, texture->vk_image, CRUDE_GFX_RESOURCE_STATE_UNDEFINED, CRUDE_GFX_RESOURCE_STATE_RENDER_TARGET, 0, 1, resource->resource_info.texture.format == VK_FORMAT_D32_SFLOAT );
        }
      }
    }
    
    {
      crude_gfx_rect2d_int scissor = {
        .x = 0, 
        .y = 0,
        .width = width, 
        .height = height
      };
      crude_gfx_cmd_set_scissor( gpu_commands, &scissor );
    }

    {
      crude_gfx_viewport viewport = { 
        viewport.rect = ( crude_gfx_rect2d_int ){ 
          .x = 0, 
          .y = 0,
          .width = width,
          .height = height
        },
        viewport.min_depth = 0.0f,
        viewport.max_depth = 1.0f,
      };
      crude_gfx_cmd_set_viewport( gpu_commands, &viewport );
    }
    
    node->graph_render_pass.pre_render( node->graph_render_pass.ctx, gpu_commands );
    crude_gfx_cmd_bind_render_pass( gpu_commands, node->render_pass, node->framebuffer, true );
    node->graph_render_pass.render( node->graph_render_pass.ctx, gpu_commands );
    crude_gfx_cmd_end_render_pass( gpu_commands );
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
  uint64 key = stbds_hash_bytes( ( void* )name, strlen( name ), 0 );
  int64 handle_index = CRUDE_HASHMAP_GET_INDEX( builder->render_pass_cache.render_pass_map, key );
  if (handle_index >= 0 )
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
  
  CRUDE_HASHMAP_PUT( builder->render_pass_cache.render_pass_map, key, render_pass );

  crude_gfx_render_graph_node *node = crude_gfx_render_graph_builder_access_node( builder, ( crude_gfx_render_graph_node_handle ){ handle_index } );
  node->graph_render_pass = render_pass;
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
  node->name = creation->name;
  node->enabled = creation->enabled;
  node->framebuffer = CRUDE_GFX_FRAMEBUFFER_HANDLE_INVALID;
  node->render_pass = CRUDE_GFX_RENDER_PASS_HANDLE_INVALID;
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->inputs, CRUDE_ARRAY_LENGTH( creation->inputs ), builder->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->outputs, CRUDE_ARRAY_LENGTH( creation->outputs ), builder->allocator_container );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->edges, CRUDE_ARRAY_LENGTH( creation->outputs ), builder->allocator_container );
  
  {
    uint64 key = stbds_hash_bytes( ( void* )node->name, strlen( node->name ), 0 );
    hmput( builder->node_cache.node_map, key, node_handle.index );
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

  crude_gfx_render_graph_resource* resource = crude_resource_pool_access_resource( &builder->resource_cache.resources, resource_handle.index );
  resource->name = creation->name;
  resource->type = creation->type;
  if ( creation->type != CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_REFERENCE )
  {
    resource->resource_info = creation->resource_info;
    resource->output_handle = resource_handle;
    resource->producer = producer;
    resource->ref_count = 0;
    
    uint64 key = stbds_hash_bytes( ( void* )resource->name, strlen( creation->name ), 0 );
    hmput( builder->resource_cache.resource_map, key, resource_handle.index );
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
  
  crude_gfx_render_graph_resource* resource = crude_resource_pool_access_resource( &builder->resource_cache.resources, resource_handle.index );
  resource->resource_info = ( crude_gfx_render_graph_resource_info ){ 0 };
  resource->producer = CRUDE_GFX_RENDER_GRAPH_NODE_HANDLE_INVALID;
  resource->output_handle = CRUDE_GFX_RENDER_GRAPH_RESOURCE_HANDLE_INVALID;
  resource->type = creation->type;
  resource->name = creation->name;
  resource->ref_count = 0;
  
  return resource_handle;
}

crude_gfx_render_graph_node*
crude_gfx_render_graph_builder_access_node
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_render_graph_node_handle                  handle
)
{
  return crude_resource_pool_access_resource( &builder->node_cache.nodes, handle.index );
}

crude_gfx_render_graph_node*
crude_gfx_render_graph_builder_access_node_by_name
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ char const                                         *name
)
{
  uint64 key = stbds_hash_bytes( ( void* )name, strlen( name ), 0 );
  uint32 handle_index = hmgeti( builder->node_cache.node_map, key );
  if ( handle_index < 0 )
  {
    return NULL;
  }
  
  return crude_resource_pool_access_resource( &builder->node_cache.nodes, ( crude_gfx_render_graph_resource_handle ){ hmget( builder->node_cache.node_map, handle_index ) }.index );

}

crude_gfx_render_graph_node_handle
crude_gfx_render_graph_builder_obtain_node
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  return ( crude_gfx_render_graph_node_handle ){ crude_resource_pool_obtain_resource( &builder->node_cache.nodes ) };
}


crude_gfx_render_graph_resource_handle
crude_gfx_render_graph_builder_obtain_resource
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  return ( crude_gfx_render_graph_resource_handle ){ crude_resource_pool_obtain_resource( &builder->resource_cache.resources ) };
}

crude_gfx_render_graph_resource*
crude_gfx_render_graph_builder_access_resource
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ crude_gfx_render_graph_resource_handle              handle
)
{
  return crude_resource_pool_access_resource( &builder->resource_cache.resources, handle.index );
}

crude_gfx_render_graph_resource*
crude_gfx_render_graph_builder_access_resource_by_name
(
  _In_ crude_gfx_render_graph_builder                     *builder,
  _In_ char const                                         *name
)
{
  uint64 key = stbds_hash_bytes( ( void* )name, strlen( name ), 0 );
  uint32 handle_index = hmgeti( builder->resource_cache.resource_map, key );
  if ( handle_index < 0 )
  {
    return NULL;
  }
  
  return crude_resource_pool_access_resource( &builder->resource_cache.resources, ( crude_gfx_render_graph_resource_handle ){ hmget( builder->resource_cache.resource_map, handle_index ) }.index );
}

void
crude_gfx_render_graph_builder_resource_cache_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  crude_resource_pool_initialize( &builder->resource_cache.resources, builder->allocator_container, CRUDE_GFX_RENDER_GRAPH_MAX_RESOURCES_COUNT, sizeof( crude_gfx_render_graph_node ) );
  builder->resource_cache.resource_map = NULL;
}

void
crude_gfx_render_graph_builder_node_cache_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  crude_resource_pool_initialize( &builder->node_cache.nodes, builder->allocator_container, CRUDE_GFX_RENDER_GRAPH_MAX_NODES_COUNT, sizeof( crude_gfx_render_graph_node ) );
  builder->node_cache.node_map = NULL;
}

void
crude_gfx_render_graph_builder_render_pass_cache_initialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  builder->render_pass_cache.render_pass_map = NULL;
}

void
crude_gfx_render_graph_builder_resource_cache_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  for ( int32 i = 0; i < hmlen( builder->resource_cache.resource_map ); ++i )
  {
    crude_gfx_render_graph_resource *resource = crude_gfx_render_graph_builder_access_resource( builder, ( crude_gfx_render_graph_resource_handle ){ builder->resource_cache.resource_map[ i ].value } );

    bool is_texture_type = ( resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_TEXTURE || resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_ATTACHMENT );
    bool is_buffer_type = ( resource->type == CRUDE_GFX_RENDER_GRAPH_RESOURCE_TYPE_BUFFER );
    if ( is_texture_type && CRUDE_RESOURCE_HANDLE_IS_VALID( resource->resource_info.texture.texture ) )
    {
      crude_gfx_texture *texture = crude_gfx_access_texture( builder->gpu, resource->resource_info.texture.texture );
      crude_gfx_destroy_texture( builder->gpu, texture->handle );
    }
    else if ( is_buffer_type && CRUDE_RESOURCE_HANDLE_IS_VALID( resource->resource_info.buffer.buffer ) )
    {
      crude_gfx_buffer *buffer = crude_gfx_access_buffer( builder->gpu, resource->resource_info.buffer.buffer );
      crude_gfx_destroy_buffer( builder->gpu, buffer->handle );
    }
  }

  crude_resource_pool_free_all_resource( &builder->resource_cache.resources );
  crude_resource_pool_deinitialize( &builder->resource_cache.resources );
  hmfree( builder->resource_cache.resource_map );
}

void
crude_gfx_render_graph_builder_node_cache_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  crude_resource_pool_free_all_resource( &builder->node_cache.nodes );
  crude_resource_pool_deinitialize( &builder->node_cache.nodes );
  hmfree( builder->node_cache.node_map );
}

void
crude_gfx_render_graph_builder_render_pass_cache_deinitialize
(
  _In_ crude_gfx_render_graph_builder                     *builder
)
{
  hmfree( builder->render_pass_cache.render_pass_map );
}

/************************************************
 *
 * Render Graph Utils Static Functions Implementation
 * 
 ***********************************************/
crude_gfx_render_graph_resource_type
string_to_resource_type_
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

crude_gfx_render_pass_operation
string_to_render_pass_operation_
(
  _In_ char const                                         *op
)
{
  if ( strcmp( op, "VK_ATTACHMENT_LOAD_OP_CLEAR" ) == 0 )
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