#include <cgltf.h>
#include <stb_ds.h>
#include <stb_image.h>

#include <crude_shaders/main.frag.inl>
#include <crude_shaders/main.vert.inl>

#include <core/assert.h>
#include <core/log.h>
#include <core/file.h>
#include <graphics/renderer.h>

#include <resources/gltf_loader.h>

static void
crude_get_mesh_vertex_buffer
(
  _In_ cgltf_data             *gltf,
  _In_ crude_scene            *scene,
  _In_ int32                   accessor_index,
  _Out_ crude_buffer_handle   *buffer_handle,
  _Out_ uint32                *buffer_offset
)
{
  if ( accessor_index == -1 )
  {
    return;
  }

  cgltf_accessor *buffer_accessor = &gltf->accessors[ accessor_index ];
  cgltf_buffer_view *buffer_view = buffer_accessor->buffer_view;
  crude_buffer_resource *buffer_gpu = &scene->buffers[ cgltf_buffer_view_index( gltf, buffer_accessor->buffer_view ) ];
  
  *buffer_handle = buffer_gpu->handle;
  *buffer_offset = buffer_accessor->offset;
}

static bool
get_mesh_material
(
  _In_ cgltf_data             *gltf,
  _In_ crude_renderer         *renderer,
  _In_ crude_scene            *scene,
  _In_ cgltf_material         *material,
  _In_ crude_mesh_draw        *mesh_draw
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
    mesh_draw->flags |= CRUDE_DRAW_FLAGS_ALPHA_MASK;
    transparent = true;
  }

  if ( material->pbr_metallic_roughness.base_color_texture.texture )
  {
    cgltf_texture *albedo_texture = material->pbr_metallic_roughness.base_color_texture.texture;
    crude_texture_resource *albedo_texture_gpu = &scene->images[ cgltf_image_index( gltf, albedo_texture->image ) ];
    crude_sampler_resource *albedo_sampler_gpu = &scene->samplers[ cgltf_sampler_index( gltf, albedo_texture->sampler ) ];
  
    mesh_draw->albedo_texture_index = albedo_texture_gpu->handle.index;
    crude_gfx_link_texture_sampler( renderer->gpu, albedo_texture_gpu->handle, albedo_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->albedo_texture_index = CRUDE_INVALID_TEXTURE_INDEX;
  }
  
  if ( material->pbr_metallic_roughness.metallic_roughness_texture.texture )
  {
    cgltf_texture *roughness_texture = material->pbr_metallic_roughness.metallic_roughness_texture.texture;
    crude_texture_resource *roughness_texture_gpu = &scene->images[ cgltf_image_index( gltf, roughness_texture->image ) ];
    crude_sampler_resource *roughness_sampler_gpu = &scene->samplers[ cgltf_sampler_index( gltf, roughness_texture->sampler ) ];
    
    mesh_draw->roughness_texture_index = roughness_texture_gpu->handle.index;
    crude_gfx_link_texture_sampler( renderer->gpu, roughness_texture_gpu->handle, roughness_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->roughness_texture_index = CRUDE_INVALID_TEXTURE_INDEX;
  }

  if ( material->occlusion_texture.texture )
  {
    cgltf_texture *occlusion_texture = material->occlusion_texture.texture;
    crude_texture_resource *occlusion_texture_gpu = &scene->images[ cgltf_image_index( gltf, occlusion_texture->image ) ];
    crude_sampler_resource *occlusion_sampler_gpu = &scene->samplers[ cgltf_sampler_index( gltf, occlusion_texture->sampler ) ];
    
    mesh_draw->occlusion_texture_index = occlusion_texture_gpu->handle.index;
    mesh_draw->metallic_roughness_occlusion_factor.z = material->occlusion_texture.scale;
    
    crude_gfx_link_texture_sampler( renderer->gpu, occlusion_texture_gpu->handle, occlusion_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->occlusion_texture_index = CRUDE_INVALID_TEXTURE_INDEX;
  }
  
  if ( material->normal_texture.texture )
  {
    cgltf_texture *normal_texture = material->normal_texture.texture;
    crude_texture_resource *normal_texture_gpu = &scene->images[ cgltf_image_index( gltf, normal_texture->image ) ];
    crude_sampler_resource *normal_sampler_gpu = &scene->samplers[ cgltf_sampler_index( gltf, normal_texture->sampler ) ];
    
    mesh_draw->normal_texture_index = normal_texture_gpu->handle.index;
    crude_gfx_link_texture_sampler( renderer->gpu, normal_texture_gpu->handle, normal_sampler_gpu->handle );
  }
  else
  {
    mesh_draw->normal_texture_index = CRUDE_INVALID_TEXTURE_INDEX;
  }
  
  crude_buffer_creation buffer_creation = {
    .usage = CRUDE_RESOURCE_USAGE_TYPE_DYNAMIC,
    .type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    .size = sizeof ( crude_shader_mesh_constants ),
    .name = "mesh_data"
  };
  mesh_draw->material_buffer = crude_gfx_create_buffer( renderer->gpu, &buffer_creation );
  
  return transparent;
}

void
crude_load_gltf_from_file
(
  _In_ crude_renderer  *renderer,
  _In_ char const      *path,
  _Out_ crude_scene    *scene
)
{
  cgltf_options gltf_options = { 0 };
  cgltf_data *gltf = NULL;
  cgltf_result result = cgltf_parse_file( &gltf_options, path, &gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", path );
  }
  
  result = cgltf_load_buffers( &gltf_options, gltf, path );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to load buffers from gltf file: %s", path );
  }
  
  result = cgltf_validate( gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to validate gltf file: %s", path );
  }

  crude_pipeline_creation pipeline_creation;
  memset( &pipeline_creation, 0, sizeof( pipeline_creation ) );
  pipeline_creation.shaders.name = "triangle";
  pipeline_creation.shaders.spv_input = true;
  pipeline_creation.shaders.stages[ 0 ].code = crude_compiled_shader_main_vert;
  pipeline_creation.shaders.stages[ 0 ].code_size = sizeof( crude_compiled_shader_main_vert );
  pipeline_creation.shaders.stages[ 0 ].type = VK_SHADER_STAGE_VERTEX_BIT;
  pipeline_creation.shaders.stages[ 1 ].code = crude_compiled_shader_main_frag;
  pipeline_creation.shaders.stages[ 1 ].code_size = sizeof( crude_compiled_shader_main_frag );
  pipeline_creation.shaders.stages[ 1 ].type = VK_SHADER_STAGE_FRAGMENT_BIT;
  pipeline_creation.shaders.stages_count = 2u;
  
  pipeline_creation.vertex_input.vertex_attributes[ 0 ] = ( crude_vertex_attribute ){ 0, 0, 0, CRUDE_VERTEX_COMPONENT_FORMAT_FLOAT3 }; // position
  pipeline_creation.vertex_input.vertex_streams[ 0 ] = ( crude_vertex_stream ){ 0, 12, CRUDE_VERTEX_INPUT_RATE_PER_VERTEX };
  
  pipeline_creation.vertex_input.vertex_attributes[ 1 ] = ( crude_vertex_attribute ){ 1, 1, 0, CRUDE_VERTEX_COMPONENT_FORMAT_FLOAT4 }; // tangent
  pipeline_creation.vertex_input.vertex_streams[ 1 ] = ( crude_vertex_stream ){ 1, 16, CRUDE_VERTEX_INPUT_RATE_PER_VERTEX};
  
  pipeline_creation.vertex_input.vertex_attributes[ 2 ] = ( crude_vertex_attribute ) { 2, 2, 0, CRUDE_VERTEX_COMPONENT_FORMAT_FLOAT3 }; // normal
  pipeline_creation.vertex_input.vertex_streams[ 2 ] = ( crude_vertex_stream ){ 2, 12, CRUDE_VERTEX_INPUT_RATE_PER_VERTEX };
  
  pipeline_creation.vertex_input.vertex_attributes[ 3 ] = ( crude_vertex_attribute ){ 3, 3, 0, CRUDE_VERTEX_COMPONENT_FORMAT_FLOAT2 }; // texcoord
  pipeline_creation.vertex_input.vertex_streams[ 3 ] = ( crude_vertex_stream ){ 3, 8, CRUDE_VERTEX_INPUT_RATE_PER_VERTEX };
  
  pipeline_creation.vertex_input.num_vertex_attributes = 4;
  pipeline_creation.vertex_input.num_vertex_streams = 4;
  
  pipeline_creation.depth_stencil.depth_write_enable = true;
  pipeline_creation.depth_stencil.depth_enable = true;
  pipeline_creation.depth_stencil.depth_comparison = VK_COMPARE_OP_LESS;

  crude_program_creation program_creation = { .pipeline_creation = pipeline_creation };
  scene->program = crude_gfx_renderer_create_program( renderer, &program_creation );
  crude_material_creation material_creatoin = { 
    .name = "material1",
    .program = scene->program,
    .render_index = 0,
  };

  scene->material = crude_gfx_renderer_create_material( renderer, &material_creatoin );

  char prev_directory[ 1024 ];
  crude_get_current_working_directory( &prev_directory, sizeof( prev_directory ) );
  
  char gltf_base_path[ 1024 ];
  memcpy( gltf_base_path, path, sizeof( gltf_base_path ) );
  crude_file_directory_from_path( gltf_base_path );

  crude_change_working_directory( gltf_base_path );

  scene->images = NULL;
  arrsetcap( scene->images, gltf->images_count );
  
  for ( uint32 image_index = 0; image_index < gltf->images_count; ++image_index )
  {
    cgltf_image *image = &gltf->images[ image_index ];
    int comp, width, height;
    uint8_t* image_data = stbi_load( image->uri, &width, &height, &comp, 4 );
  
    if ( !image_data )
    {
      continue;
    }

    crude_texture_creation texture_creation = {
      .initial_data = image_data,
      .width = width,
      .height = height,
      .depth = 1u,
      .mipmaps = 1u,
      .flags = 0u,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .type = CRUDE_TEXTURE_TYPE_TEXTURE_2D,
      .name = image->uri,
    };

    crude_texture_resource *texture_resource = crude_gfx_renderer_create_texture( renderer, &texture_creation );
    free( image_data );

    arrpush( scene->images, *texture_resource );
  }

  // Load all samplers
  scene->samplers = NULL;
  arrsetcap( scene->samplers, gltf->samplers_count );

  for ( uint32 sampler_index = 0; sampler_index < gltf->samplers_count; ++sampler_index )
  {
    cgltf_sampler *sampler = &gltf->samplers[ sampler_index ];
  
    crude_sampler_creation creation;
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
    
    crude_sampler_resource *sampler_resource = crude_gfx_renderer_create_sampler( renderer, &creation );
    arrpush( scene->samplers, *sampler_resource );
  }
  
  scene->buffers = NULL;
  arrsetcap( scene->buffers, gltf->buffer_views_count );

  for ( uint32 buffer_view_index = 0; buffer_view_index < gltf->buffer_views_count; ++buffer_view_index )
  {
    cgltf_buffer_view *buffer_view = &gltf->buffer_views[ buffer_view_index ];
    cgltf_buffer *buffer = buffer_view->buffer;
  
    uint8* data = ( uint8* )buffer->data + buffer_view->offset;
  
    if ( buffer_view->name == NULL )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_FILEIO, "Bufer name is null: %u", buffer_view_index );
    }

    crude_buffer_creation buffer_creation = {
      .initial_data = data,
      .usage = CRUDE_RESOURCE_USAGE_TYPE_IMMUTABLE,
      .size = buffer_view->size,
      .type_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .name = buffer->name
    };
    crude_buffer_resource *buffer_resource = crude_gfx_renderer_create_buffer( renderer, &buffer_creation );
    arrpush( scene->buffers, *buffer_resource );
  }

  scene->mesh_draws = NULL;
  arrsetcap( scene->mesh_draws, gltf->meshes_count );

  cgltf_scene *root_scene = gltf->scene;

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
  
    for ( uint32 primitive_index = 0; primitive_index < node->mesh->primitives_count; ++primitive_index )
    {
      crude_mesh_draw mesh_draw = { .scale = node_scale };
      
      cgltf_primitive *mesh_primitive = &node->mesh->primitives[ primitive_index ];
      
      for ( uint32 i = 0; i < mesh_primitive->attributes_count; ++i )
      {
        crude_buffer_resource *buffer_gpu = &scene->buffers[ cgltf_buffer_view_index( gltf, mesh_primitive->attributes[ i ].data->buffer_view ) ];
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
      
      cgltf_accessor *indices_accessor = mesh_primitive->indices;
      cgltf_buffer_view *indices_buffer_view = indices_accessor->buffer_view;
      
      crude_buffer_resource *indices_buffer_gpu = &scene->buffers[ cgltf_buffer_view_index( gltf, indices_accessor->buffer_view ) ];
      mesh_draw.index_buffer = indices_buffer_gpu->handle;
      mesh_draw.index_offset = indices_accessor->offset;
      mesh_draw.primitive_count = indices_accessor->count;

      bool transparent = get_mesh_material( gltf, renderer, scene, mesh_primitive->material, &mesh_draw );
      
      if ( transparent)
      {
        if ( mesh_primitive->material->double_sided )
        {
          //CRUDE_ABORT( CRUDE_CHANNEL_FILEIO, "Can't handle such type of material!" );
          //mesh_draw.material = material_no_cull_transparent;
        }
        else
        {
          //CRUDE_ABORT( CRUDE_CHANNEL_FILEIO, "Can't handle such type of material!" );
          //mesh_draw.material = material_cull_transparent;
        }
      }
      else
      {
        if ( mesh_primitive->material->double_sided )
        {
          //CRUDE_ABORT( CRUDE_CHANNEL_FILEIO, "Can't handle such type of material!" );
          //mesh_draw.material = material_no_cull_opaque;
        }
        else
        {
          //CRUDE_ABORT( CRUDE_CHANNEL_FILEIO, "Can't handle such type of material!" );
          //mesh_draw.material = material_cull_opaque;
        }
      }
      mesh_draw.material = scene->material;
      arrpush( scene->mesh_draws, mesh_draw );
    }
  }
  
  crude_change_working_directory( prev_directory );
  cgltf_free( gltf );
}

void
crude_unload_gltf_from_file
(
  _In_ crude_renderer  *renderer,
  _In_ crude_scene     *scene
)
{
  crude_gfx_renderer_destroy_program( renderer, scene->program );
  crude_gfx_renderer_destroy_material( renderer, scene->material );

  for ( uint32 i = 0; i < arrlen( scene->images ); ++i )
  {
    crude_gfx_renderer_destroy_texture( renderer, &scene->images[ i ] );
  }
  arrfree( scene->images );

  for ( uint32 i = 0; i < arrlen( scene->samplers ); ++i )
  {
    crude_gfx_renderer_destroy_sampler( renderer, &scene->samplers[ i ] );
  }
  arrfree( scene->samplers );
  
  for ( uint32 i = 0; i < arrlen( scene->buffers ); ++i )
  {
    crude_gfx_renderer_destroy_buffer( renderer, &scene->buffers[ i ] );
  }
  arrfree( scene->buffers );

  for ( uint32 i = 0; i < arrlen( scene->mesh_draws ); ++i )
  {
     crude_gfx_destroy_buffer( renderer->gpu, scene->mesh_draws[ i ].material_buffer );
  }
  arrfree( scene->mesh_draws );

  arrfree( scene->buffers );
}