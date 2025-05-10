#include <cJSON.h>
#include <cgltf.h>
#include <stb_image.h>

#include <core/file.h>
#include <core/string.h>

#include <graphics/renderer_resources_loader.h>

/************************************************
 *
 * GLTF Utils Functinos Declaration
 * 
 ***********************************************/
static void
_get_gltf_mesh_vertex_buffer
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gfx_renderer_scene                           *scene,
  _In_ int32                                               accessor_index,
  _Out_ crude_gfx_buffer_handle                           *buffer_handle,
  _Out_ uint32                                            *buffer_offset
);

static bool
_create_gltf_mesh_material
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_scene                           *scene,
  _In_ cgltf_material                                     *material,
  _In_ crude_gfx_mesh                                     *mesh_draw
);

/************************************************
 *
 * Local Renderer Resources Loader Functions Declaration.
 * 
 ***********************************************/
static void
parse_gpu_pipeline_
(
  cJSON const                                             *pipeline_json,
  crude_gfx_pipeline_creation                             *pipeline_creation,
  crude_string_buffer                                     *path_buffer,
  crude_gfx_render_graph                                  *render_graph,
  crude_stack_allocator                                   *temporary_allocator
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
        parse_gpu_pipeline_( pipeline, &pipeline_creation, &path_buffer, render_graph, temporary_allocator );
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

/************************************************
 *
 * Renderer Scene
 * 
 ***********************************************/
void
crude_gfx_renderer_scene_load_from_file
(
  _In_ crude_gfx_renderer_scene                          *scene,
  _In_ char const                                        *path,
  _In_ crude_stack_allocator                             *temporary_allocator
)
{
  cgltf_options                                            gltf_options;
  crude_gfx_renderer_material_creation                     material_creatoin;
  crude_allocator_container                                temporary_allocator_container;
  crude_string_buffer                                      temporary_string_buffer;
  cgltf_result                                             result;
  cgltf_data                                              *gltf;
  cgltf_scene                                             *root_scene;
  char                                                     prev_directory[ 1024 ];
  char                                                     gltf_base_path[ 1024 ];
  uint32                                                   temporary_allocator_mark;

  CRUDE_ASSERT( strcpy_s( scene->path, sizeof( scene->path ), path ) == 0 );

  temporary_allocator_container = crude_stack_allocator_pack( temporary_allocator );
  temporary_allocator_mark = crude_stack_allocator_get_marker( temporary_allocator );

  crude_string_buffer_initialize( &temporary_string_buffer, 1024, temporary_allocator_container );

  gltf_options = ( cgltf_options ){ 
    .memory = {
      .alloc_func = temporary_allocator_container.allocate,
      .free_func  = temporary_allocator_container.deallocate,
      .user_data = temporary_allocator_container.ctx
    },
  };
  
  gltf = NULL;
  result = cgltf_parse_file( &gltf_options, scene->path, &gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", scene->path );
  }
  
  result = cgltf_load_buffers( &gltf_options, gltf, scene->path );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to load buffers from gltf file: %s", scene->path );
  }
  
  result = cgltf_validate( gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to validate gltf file: %s", scene->path );
  }

  crude_get_current_working_directory( &prev_directory, sizeof( prev_directory ) );
  memcpy( gltf_base_path, scene->path, sizeof( gltf_base_path ) );
  crude_file_directory_from_path( gltf_base_path );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene->images, gltf->images_count, scene->allocator_container );
  for ( uint32 image_index = 0; image_index < gltf->images_count; ++image_index )
  {
    crude_gfx_renderer_texture                            *texture_resource;
    crude_gfx_texture_creation                             texture_creation;
    cgltf_image const                                     *image;
    char                                                   image_full_filename[ 512 ] = { 0 };
    int                                                    comp, width, height;
    
    image = &gltf->images[ image_index ];
    strcat( image_full_filename, gltf_base_path );
    strcat( image_full_filename, image->uri );
    stbi_info( image_full_filename, &width, &height, &comp );

    texture_creation = crude_gfx_texture_creation_empty();
    texture_creation.initial_data = NULL;
    texture_creation.width = width;
    texture_creation.height = height;
    texture_creation.depth = 1u;
    texture_creation.mipmaps = 1u;
    texture_creation.flags = 0u;
    texture_creation.format = VK_FORMAT_R8G8B8A8_UNORM;
    texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
    texture_creation.name = image_full_filename;

    texture_resource = crude_gfx_renderer_create_texture( scene->renderer, &texture_creation );
    CRUDE_ARRAY_PUSH( scene->images, *texture_resource );
    crude_gfx_asynchronous_loader_request_texture_data( scene->async_loader, image_full_filename, texture_resource->handle );
  }
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene->samplers, gltf->samplers_count, scene->allocator_container );
  for ( uint32 sampler_index = 0; sampler_index < gltf->samplers_count; ++sampler_index )
  {
    crude_gfx_renderer_sampler                            *sampler_resource;
    crude_gfx_sampler_creation                             creation;
    cgltf_sampler                                         *sampler;

    sampler = &gltf->samplers[ sampler_index ];

    creation = crude_gfx_sampler_creation_empty();
    switch ( sampler->min_filter )
    {
    case cgltf_filter_type_nearest:
      creation.min_filter = VK_FILTER_NEAREST;
      break;
    case cgltf_filter_type_linear:
      creation.min_filter = VK_FILTER_LINEAR;
      break;
    case cgltf_filter_type_linear_mipmap_nearest:
      creation.min_filter = VK_FILTER_LINEAR;
      creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
      break;
    case cgltf_filter_type_linear_mipmap_linear:
      creation.min_filter = VK_FILTER_LINEAR;
      creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      break;
    case cgltf_filter_type_nearest_mipmap_nearest:
      creation.min_filter = VK_FILTER_NEAREST;
      creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
      break;
    case cgltf_filter_type_nearest_mipmap_linear:
      creation.min_filter = VK_FILTER_NEAREST;
      creation.mip_filter = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      break;
    }
    
    creation.mag_filter = sampler->mag_filter == cgltf_filter_type_linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    
    switch ( sampler->wrap_s )
    {
      case cgltf_wrap_mode_clamp_to_edge:
        creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;
      case cgltf_wrap_mode_mirrored_repeat:
        creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        break;
      case cgltf_wrap_mode_repeat:
        creation.address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    }
    
    switch ( sampler->wrap_t )
    {
    case cgltf_wrap_mode_clamp_to_edge:
      creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      break;
    case cgltf_wrap_mode_mirrored_repeat:
      creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
      break;
    case cgltf_wrap_mode_repeat:
      creation.address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      break;
    }

    creation.address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    
    creation.name = "";
    
    sampler_resource = crude_gfx_renderer_create_sampler( scene->renderer, &creation );
    CRUDE_ARRAY_PUSH( scene->samplers, *sampler_resource );
  }
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene->buffers, gltf->buffer_views_count, scene->allocator_container );
  for ( uint32 buffer_view_index = 0; buffer_view_index < gltf->buffer_views_count; ++buffer_view_index )
  {
    cgltf_buffer_view const                               *buffer_view;
    cgltf_buffer const                                    *buffer;
    crude_gfx_buffer_creation                              cpu_buffer_creation;
    crude_gfx_buffer_creation                              gpu_buffer_creation;
    crude_gfx_buffer_handle                                cpu_buffer;
    crude_gfx_renderer_buffer                             *gpu_buffer_resource;
    uint8                                                 *buffer_data;
    char const                                            *buffer_name;

    buffer_view = &gltf->buffer_views[ buffer_view_index ];
    buffer = buffer_view->buffer;
  
    buffer_data = ( uint8* )buffer->data + buffer_view->offset;
  
    if ( buffer_view->name == NULL )
    {
      buffer_name = crude_string_buffer_append_use_f( &temporary_string_buffer, "renderer_scene_buffer%i", buffer_view_index );
    }
    else
    {
      buffer_name = buffer_view->name;
    }
    
    cpu_buffer_creation = crude_gfx_buffer_creation_empty();
    cpu_buffer_creation.initial_data = buffer_data;
    cpu_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    cpu_buffer_creation.size = buffer_view->size;
    cpu_buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    cpu_buffer_creation.name = buffer_name;
    cpu_buffer = crude_gfx_create_buffer( scene->renderer->gpu, &cpu_buffer_creation );

    gpu_buffer_creation = crude_gfx_buffer_creation_empty();
    gpu_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    gpu_buffer_creation.size = buffer_view->size;
    gpu_buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    gpu_buffer_creation.name = buffer_name;
    gpu_buffer_creation.device_only = true;
    gpu_buffer_resource = crude_gfx_renderer_create_buffer( scene->renderer, &gpu_buffer_creation );
    CRUDE_ARRAY_PUSH( scene->buffers, *gpu_buffer_resource );

    crude_gfx_asynchronous_loader_request_buffer_copy( scene->async_loader, cpu_buffer, gpu_buffer_resource->handle );

    crude_string_buffer_clear( &temporary_string_buffer );
  }
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( scene->meshes, gltf->meshes_count, scene->allocator_container );

  root_scene = gltf->scene;
  for ( uint32 node_index = 0; node_index < root_scene->nodes_count; ++node_index )
  {
    cgltf_node *node = root_scene->nodes[ node_index ];
  
    if ( !node->mesh )
    {
      continue;
    }
  
    crude_float3 node_scale = { 1.0f, 1.0f, 1.0f };
    if ( node->has_scale )
    {
      node_scale = ( crude_float3 ){ node->scale[0], node->scale[1], node->scale[2] };
    }
    crude_float3 node_translation = { 1.0f, 1.0f, 1.0f };
    if ( node->has_translation )
    {
      node_translation = ( crude_float3 ){ node->translation[0], node->translation[1], node->translation[2] };
    }
    crude_float4 node_rotation = { 1.0f, 1.0f, 1.0f };
    if ( node->has_rotation )
    {
      node_rotation = ( crude_float4 ){ node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[4] };
    }
  
    for ( uint32 primitive_index = 0; primitive_index < node->mesh->primitives_count; ++primitive_index )
    {
      crude_gfx_mesh                                      mesh_draw;
      cgltf_primitive                                     *mesh_primitive;
      cgltf_accessor                                      *indices_accessor;
      cgltf_buffer_view                                   *indices_buffer_view;
      crude_gfx_renderer_buffer                           *indices_buffer_gpu;
      bool                                                 material_transparent;
      
      mesh_primitive = &node->mesh->primitives[ primitive_index ];

      mesh_draw = ( crude_gfx_mesh ){ 0 };
      for ( uint32 i = 0; i < mesh_primitive->attributes_count; ++i )
      {
        crude_gfx_renderer_buffer *buffer_gpu = &scene->buffers[ cgltf_buffer_view_index( gltf, mesh_primitive->attributes[ i ].data->buffer_view ) ];
        switch ( mesh_primitive->attributes[ i ].type )
        {
        case cgltf_attribute_type_position:
        {
          mesh_draw.position_buffer = buffer_gpu->handle;
          mesh_draw.position_offset = mesh_primitive->attributes[ i ].data->offset;
          break;
        }
        case cgltf_attribute_type_tangent:
        {
          mesh_draw.tangent_buffer = buffer_gpu->handle;
          mesh_draw.tangent_offset = mesh_primitive->attributes[ i ].data->offset;
          break;
        }
        case cgltf_attribute_type_normal:
        {
          mesh_draw.normal_buffer = buffer_gpu->handle;
          mesh_draw.normal_offset = mesh_primitive->attributes[ i ].data->offset;
          break;
        }
        case cgltf_attribute_type_texcoord:
        {
          mesh_draw.texcoord_buffer = buffer_gpu->handle;
          mesh_draw.texcoord_offset = mesh_primitive->attributes[ i ].data->offset;
          break;
        }
        }
      }
      
      indices_accessor = mesh_primitive->indices;
      indices_buffer_view = indices_accessor->buffer_view;
      
      indices_buffer_gpu = &scene->buffers[ cgltf_buffer_view_index( gltf, indices_accessor->buffer_view ) ];

      material_transparent = _create_gltf_mesh_material( gltf, scene->renderer, scene, mesh_primitive->material, &mesh_draw );
      
      //!TODO
      //if ( transparent)
      //{
      //  if ( mesh_primitive->material->double_sided )
      //  {
      //    //CRUDE_ABORT( CRUDE_CHANNEL_FILEIO, "Can't handle such type of material!" );
      //    //mesh_draw.material = material_no_cull_transparent;
      //  }
      //  else
      //  {
      //    //CRUDE_ABORT( CRUDE_CHANNEL_FILEIO, "Can't handle such type of material!" );
      //    //mesh_draw.material = material_cull_transparent;
      //  }
      //}
      //else
      //{
      //  if ( mesh_primitive->material->double_sided )
      //  {
      //    //CRUDE_ABORT( CRUDE_CHANNEL_FILEIO, "Can't handle such type of material!" );
      //    //mesh_draw.material = material_no_cull_opaque;
      //  }
      //  else
      //  {
      //    //CRUDE_ABORT( CRUDE_CHANNEL_FILEIO, "Can't handle such type of material!" );
      //    //mesh_draw.material = material_cull_opaque;
      //  }
      //}
      mesh_draw.scale = node_scale;
      mesh_draw.translation = node_translation;
      mesh_draw.rotation = node_rotation;
      mesh_draw.index_buffer = indices_buffer_gpu->handle;
      mesh_draw.index_offset = indices_accessor->offset;
      mesh_draw.primitive_count = indices_accessor->count;
      CRUDE_ARRAY_PUSH( scene->meshes, mesh_draw );
    }
  }
  cgltf_free( gltf );
  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_mark );
}

/************************************************
 *
 * Local Renderer Resources Loader Functions Implementation.
 * 
 ***********************************************/
void
parse_gpu_pipeline_
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
        .source_color = get_blend_factor_( src_colour ),
        .destination_color = get_blend_factor_( dst_colour ),
        .color_operation = get_blend_op_( blend_op ),
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

/************************************************
 *
 * GLTF Utils Functinos Implementation
 * 
 ***********************************************/

void
_get_gltf_mesh_vertex_buffer
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gfx_renderer_scene                           *scene,
  _In_ int32                                               accessor_index,
  _Out_ crude_gfx_buffer_handle                           *buffer_handle,
  _Out_ uint32                                            *buffer_offset
)
{
  if ( accessor_index == -1 )
  {
    return;
  }

  cgltf_accessor *buffer_accessor = &gltf->accessors[ accessor_index ];
  cgltf_buffer_view *buffer_view = buffer_accessor->buffer_view;
  crude_gfx_renderer_buffer *buffer_gpu = &scene->buffers[ cgltf_buffer_view_index( gltf, buffer_accessor->buffer_view ) ];
  
  *buffer_handle = buffer_gpu->handle;
  *buffer_offset = buffer_accessor->offset;
}

bool
_create_gltf_mesh_material
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_renderer_scene                           *scene,
  _In_ cgltf_material                                     *material,
  _In_ crude_gfx_mesh                                     *mesh_draw
)
{
  bool transparent = false;
  
  mesh_draw->flags = 0;

  mesh_draw->base_color_factor = ( crude_float4 ) {
    material->pbr_metallic_roughness.base_color_factor[ 0 ],
    material->pbr_metallic_roughness.base_color_factor[ 1 ],
    material->pbr_metallic_roughness.base_color_factor[ 2 ],
    material->pbr_metallic_roughness.base_color_factor[ 3 ],
  };
  
  mesh_draw->metallic_roughness_occlusion_factor.x = material->pbr_metallic_roughness.roughness_factor;
  mesh_draw->metallic_roughness_occlusion_factor.y = material->pbr_metallic_roughness.metallic_factor;
  mesh_draw->alpha_cutoff = material->alpha_cutoff;
  
  if (material->alpha_mode == cgltf_alpha_mode_mask )
  {
    mesh_draw->flags |= CRUDE_GFX_DRAW_FLAGS_ALPHA_MASK;
    transparent = true;
  }

  if ( material->pbr_metallic_roughness.base_color_texture.texture )
  {
    cgltf_texture *albedo_texture = material->pbr_metallic_roughness.base_color_texture.texture;
    crude_gfx_renderer_texture *albedo_texture_gpu = &scene->images[ cgltf_image_index( gltf, albedo_texture->image ) ];
    crude_gfx_renderer_sampler *albedo_sampler_gpu = &scene->samplers[ cgltf_sampler_index( gltf, albedo_texture->sampler ) ];
  
    mesh_draw->albedo_texture_index = albedo_texture_gpu->handle.index;
    crude_gfx_link_texture_sampler( renderer->gpu, albedo_texture_gpu->handle, albedo_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->albedo_texture_index = CRUDE_GFX_RENDERER_TEXTURE_INDEX_INVALID;
  }
  
  if ( material->pbr_metallic_roughness.metallic_roughness_texture.texture )
  {
    cgltf_texture *roughness_texture = material->pbr_metallic_roughness.metallic_roughness_texture.texture;
    crude_gfx_renderer_texture *roughness_texture_gpu = &scene->images[ cgltf_image_index( gltf, roughness_texture->image ) ];
    crude_gfx_renderer_sampler *roughness_sampler_gpu = &scene->samplers[ cgltf_sampler_index( gltf, roughness_texture->sampler ) ];
    
    mesh_draw->roughness_texture_index = roughness_texture_gpu->handle.index;
    crude_gfx_link_texture_sampler( renderer->gpu, roughness_texture_gpu->handle, roughness_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->roughness_texture_index = CRUDE_GFX_RENDERER_TEXTURE_INDEX_INVALID;
  }

  if ( material->occlusion_texture.texture )
  {
    cgltf_texture *occlusion_texture = material->occlusion_texture.texture;
    crude_gfx_renderer_texture *occlusion_texture_gpu = &scene->images[ cgltf_image_index( gltf, occlusion_texture->image ) ];
    crude_gfx_renderer_sampler *occlusion_sampler_gpu = &scene->samplers[ cgltf_sampler_index( gltf, occlusion_texture->sampler ) ];
    
    mesh_draw->occlusion_texture_index = occlusion_texture_gpu->handle.index;
    mesh_draw->metallic_roughness_occlusion_factor.z = material->occlusion_texture.scale;
    
    crude_gfx_link_texture_sampler( renderer->gpu, occlusion_texture_gpu->handle, occlusion_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->occlusion_texture_index = CRUDE_GFX_RENDERER_TEXTURE_INDEX_INVALID;
  }
  
  if ( material->normal_texture.texture )
  {
    cgltf_texture *normal_texture = material->normal_texture.texture;
    crude_gfx_renderer_texture *normal_texture_gpu = &scene->images[ cgltf_image_index( gltf, normal_texture->image ) ];
    crude_gfx_renderer_sampler *normal_sampler_gpu = &scene->samplers[ cgltf_sampler_index( gltf, normal_texture->sampler ) ];
    
    mesh_draw->normal_texture_index = normal_texture_gpu->handle.index;
    crude_gfx_link_texture_sampler( renderer->gpu, normal_texture_gpu->handle, normal_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->normal_texture_index = CRUDE_GFX_RENDERER_TEXTURE_INDEX_INVALID;
  }
  
  crude_gfx_buffer_creation buffer_creation = crude_gfx_buffer_creation_empty();
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC;
  buffer_creation.type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  buffer_creation.size = sizeof ( crude_gfx_shader_mesh_constants );
  buffer_creation.name = "mesh_data";
  mesh_draw->material_buffer = crude_gfx_create_buffer( renderer->gpu, &buffer_creation );
  
  return transparent;
}