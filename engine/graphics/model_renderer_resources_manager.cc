#include <cgltf.h>
#include <stb_image.h>
#include <meshoptimizer.h>

#include <core/file.h>
#include <core/hash_map.h>

#include <graphics/model_renderer_resources_manager.h>

/**
 * Scene Renderer Register Nodes & Gltf & Lights
 */
static crude_gfx_model_renderer_resources
crude_gfx_model_renderer_resources_manager_load_gltf_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ char const                                         *gltf_path
);

static bool
crude_gfx_model_renderer_resources_manager_gltf_create_mesh_material_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ cgltf_material                                     *material,
  _In_ crude_gfx_mesh_cpu                                 *mesh_draw,
  _In_ size_t                                              images_offset,
  _In_ size_t                                              samplers_offset
);

static cgltf_data*
crude_gfx_model_renderer_resources_manager_gltf_parse_
(
  _In_ crude_heap_allocator                               *gltf_allocator,
  _In_ char const                                         *gltf_path
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_images_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_samplers_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_textures_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_buffers_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_meshes_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ uint32                                            **gltf_mesh_index_to_mesh_primitive_index,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory,
  _In_ crude_entity                                        node,
  _Out_ crude_gfx_mesh_cpu                               **meshes,
  _In_ uint32                                              buffers_offset,
  _In_ uint32                                              images_offset,
  _In_ uint32                                              samplers_offset
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlets_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _Out_ crude_gfx_mesh_cpu                                *meshes,
  _In_ uint64                                              meshes_offset
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_nodes_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_entity                                        parent_node,
  _In_ cgltf_node                                        **gltf_nodes,
  _In_ uint32                                              gltf_nodes_count,
  _In_ uint32                                             *gltf_mesh_index_to_mesh_primitive_index
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_vertices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ crude_gfx_meshlet_vertex_gpu                      **vertices
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_indices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ uint32                                            **indices,
  _In_ uint32                                              vertices_offset
);

void
crude_gfx_model_renderer_resources_manager_intialize
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
	_In_ crude_gfx_model_renderer_resources_manager_creation const *creation
)
{
}

crude_gfx_model_renderer_resources
crude_gfx_model_renderer_resources_manager_add_gltf_model
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
	_In_ char const																					*filepath
)
{
  crude_gfx_model_renderer_resources                       model_renderer_resouces;
  uint64                                                   filename_hashed;

  filename_hashed = crude_hash_string( filepath, 0 );
  if ( CRUDE_HASHMAP_GET( manager->model_hashed_name_to_model_renderer_resource, filename_hashed ) != NULL )
  {
    return;
  }

  model_renderer_resouces = crude_gfx_model_renderer_resources_manager_load_gltf_( manager, filepath );
  CRUDE_HASHMAP_SET( manager->model_hashed_name_to_model_renderer_resource, filename_hashed, model_renderer_resouces );
  return model_renderer_resouces;
}

/**
 * Register Nodes & Gltf & Lights
 */
crude_gfx_model_renderer_resources
crude_gfx_model_renderer_resources_manager_load_gltf_
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
  _In_ char const                                         *gltf_path
)
{
  cgltf_data                                              *gltf;
  uint32                                                  *gltf_mesh_index_to_mesh_primitive_index;
  crude_gfx_mesh_cpu                                      *meshes;
  crude_gfx_model_renderer_resources                       model_renderer_resouces;
  crude_string_buffer                                      temporary_string_buffer;
  char                                                     gltf_directory[ 512 ];
  uint64                                                   temporary_allocator_mark, images_offset, samplers_offset, buffers_offset, meshes_offset;

  temporary_allocator_mark = crude_stack_allocator_get_marker( manager->temporary_allocator );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( gltf_mesh_index_to_mesh_primitive_index, 0, crude_stack_allocator_pack( manager->temporary_allocator ) );
  crude_string_buffer_initialize( &temporary_string_buffer, 1024, crude_stack_allocator_pack( manager->temporary_allocator ) );
  
  images_offset = CRUDE_ARRAY_LENGTH( manager->images );
  samplers_offset = CRUDE_ARRAY_LENGTH( manager->samplers );
  buffers_offset = CRUDE_ARRAY_LENGTH( manager->buffers );
  buffers_offset = CRUDE_ARRAY_LENGTH( manager->buffers );

  /* Parse gltf */
  gltf = crude_gfx_model_renderer_resources_manager_gltf_parse_( manager->cgltf_temporary_allocator, gltf_path );
  if ( !gltf )
  {
    goto cleanup;
  }
  
  crude_memory_copy( gltf_directory, gltf_path, sizeof( gltf_directory ) );
  crude_file_directory_from_path( gltf_directory );
  
  model_renderer_resouces = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources );
  model_renderer_resouces.main_node = crude_entity_create_empty( CRUDE_CAST( ecs_world_t*, manager->world ), gltf_path );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( model_renderer_resouces.meshes_instances, 0u, crude_heap_allocator_pack( manager->allocator ) );

  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading \"%s\" images", gltf_path );
  crude_gfx_model_renderer_resources_manager_gltf_load_images_( manager, gltf, &temporary_string_buffer, gltf_directory );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading \"%s\" samplers", gltf_path );
  crude_gfx_model_renderer_resources_manager_gltf_load_samplers_( manager, gltf, &temporary_string_buffer, gltf_directory );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading \"%s\" textures", gltf_path );
  crude_gfx_model_renderer_resources_manager_gltf_load_textures_( manager, gltf ); /* Should be executed after images/samplers loaded*/
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading \"%s\" bufferse", gltf_path );
  crude_gfx_model_renderer_resources_manager_gltf_load_buffers_( manager, gltf, &temporary_string_buffer, gltf_directory );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading \"%s\" meshes", gltf_path );
  crude_gfx_model_renderer_resources_manager_gltf_load_meshes_( manager, gltf, &gltf_mesh_index_to_mesh_primitive_index, &temporary_string_buffer, gltf_directory, &meshes, buffers_offset, images_offset, samplers_offset );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Create \"%s\" meshlets", gltf_path );
  crude_gfx_model_renderer_resources_manager_gltf_load_meshlets_( manager, gltf, meshes, meshes_offset );
  
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading \"%s\" nodes", gltf_path );
  for ( uint32 i = 0; i < gltf->scenes_count; ++i )
  {
    crude_gfx_model_renderer_resources_manager_gltf_load_nodes_( manager, &model_renderer_resouces, gltf, model_renderer_resouces.main_node, gltf->scene[ i ].nodes, gltf->scene[ i ].nodes_count, gltf_mesh_index_to_mesh_primitive_index );
  }
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "\"%s\" loading finished", gltf_path );

cleanup:
  if ( gltf )
  {
    cgltf_free( gltf );
  }
  crude_stack_allocator_free_marker( manager->temporary_allocator, temporary_allocator_mark );
}


/************************************************
 *
 * GLTF Utils Functinos Implementation
 * 
 ***********************************************/
bool
crude_gfx_model_renderer_resources_manager_gltf_create_mesh_material_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ cgltf_material                                     *material,
  _In_ crude_gfx_mesh_cpu                                 *mesh_draw,
  _In_ size_t                                              images_offset,
  _In_ size_t                                              samplers_offset
)
{
  mesh_draw->flags = 0;
  mesh_draw->albedo_color_factor.x = 1;
  mesh_draw->albedo_color_factor.y = 1;
  mesh_draw->albedo_color_factor.z = 1;
  mesh_draw->albedo_color_factor.w = 1;
  mesh_draw->metallic_roughness_occlusion_factor.x = 1;
  mesh_draw->metallic_roughness_occlusion_factor.y = 0;
  mesh_draw->alpha_cutoff = 0;
  mesh_draw->albedo_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  mesh_draw->metallic_roughness_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  mesh_draw->occlusion_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;
  mesh_draw->normal_texture_handle = CRUDE_GFX_TEXTURE_HANDLE_INVALID;

  if ( material == NULL )
  {
    return false;
  }

  if ( material->pbr_metallic_roughness.base_color_factor )
  {
    mesh_draw->albedo_color_factor.x = material->pbr_metallic_roughness.base_color_factor[ 0 ];
    mesh_draw->albedo_color_factor.y = material->pbr_metallic_roughness.base_color_factor[ 1 ];
    mesh_draw->albedo_color_factor.z = material->pbr_metallic_roughness.base_color_factor[ 2 ];
    mesh_draw->albedo_color_factor.w = material->pbr_metallic_roughness.base_color_factor[ 3 ];
  }
  
  mesh_draw->metallic_roughness_occlusion_factor.x = material->pbr_metallic_roughness.metallic_factor;
  mesh_draw->metallic_roughness_occlusion_factor.y = material->pbr_metallic_roughness.roughness_factor;
  mesh_draw->alpha_cutoff = material->alpha_cutoff;
  
  if ( material->alpha_mode == cgltf_alpha_mode_mask )
  {
    mesh_draw->flags |= CRUDE_GFX_DRAW_FLAGS_ALPHA_MASK;
  }

  if ( material->pbr_metallic_roughness.base_color_texture.texture )
  {
    cgltf_texture                                         *gltf_albedo_texture;
    crude_gfx_texture_handle                               albedo_texture_handle;
    crude_gfx_sampler_handle                               albedo_sampler_handle;

    gltf_albedo_texture = material->pbr_metallic_roughness.base_color_texture.texture;
    albedo_texture_handle = manager->images[ images_offset + cgltf_image_index( gltf, gltf_albedo_texture->image ) ];
    albedo_sampler_handle = manager->samplers[ samplers_offset + cgltf_sampler_index( gltf, gltf_albedo_texture->sampler ) ];
  
    mesh_draw->albedo_texture_handle = albedo_texture_handle;
    crude_gfx_link_texture_sampler( manager->gpu, albedo_texture_handle, albedo_sampler_handle );
  }
  
  if ( material->pbr_metallic_roughness.metallic_roughness_texture.texture )
  {
    cgltf_texture                                         *gltf_roughness_texture;
    crude_gfx_texture_handle                               roughness_texture_handle;
    crude_gfx_sampler_handle                               roughness_sampler_handle;

    gltf_roughness_texture = material->pbr_metallic_roughness.metallic_roughness_texture.texture;
    roughness_texture_handle = manager->images[ images_offset + cgltf_image_index( gltf, gltf_roughness_texture->image ) ];
    roughness_sampler_handle = manager->samplers[ samplers_offset + cgltf_sampler_index( gltf, gltf_roughness_texture->sampler ) ];
    
    mesh_draw->metallic_roughness_texture_handle = roughness_texture_handle;
    crude_gfx_link_texture_sampler( manager->gpu, roughness_texture_handle, roughness_sampler_handle );
  }

  if ( material->occlusion_texture.texture )
  {
    cgltf_texture                                         *gltf_occlusion_texture;
    crude_gfx_texture_handle                               occlusion_texture_handle;
    crude_gfx_sampler_handle                               occlusion_sampler_handle;

    gltf_occlusion_texture = material->occlusion_texture.texture;
    occlusion_texture_handle = manager->images[ images_offset + cgltf_image_index( gltf, gltf_occlusion_texture->image ) ];
    occlusion_sampler_handle = manager->samplers[ samplers_offset + cgltf_sampler_index( gltf, gltf_occlusion_texture->sampler ) ];
    
    mesh_draw->occlusion_texture_handle = occlusion_texture_handle;
    mesh_draw->metallic_roughness_occlusion_factor.z = material->occlusion_texture.scale;
    
    crude_gfx_link_texture_sampler( manager->gpu, occlusion_texture_handle, occlusion_sampler_handle );
  }
  
  if ( material->normal_texture.texture )
  {
    cgltf_texture                                         *gltf_normal_texture;
    crude_gfx_texture_handle                               normal_texture_handle;
    crude_gfx_sampler_handle                               normal_sampler_handle;

    gltf_normal_texture = material->normal_texture.texture;
    normal_texture_handle = manager->images[ images_offset + cgltf_image_index( gltf, gltf_normal_texture->image ) ];
    normal_sampler_handle = manager->samplers[ samplers_offset + cgltf_sampler_index( gltf, gltf_normal_texture->sampler ) ];
    
    mesh_draw->normal_texture_handle = normal_texture_handle;
    crude_gfx_link_texture_sampler( manager->gpu, normal_texture_handle, normal_sampler_handle );
  }
  
  return true;
}

cgltf_data*
crude_gfx_model_renderer_resources_manager_gltf_parse_
(
  _In_ crude_heap_allocator                               *gltf_allocator,
  _In_ char const                                         *gltf_path
)
{
  cgltf_data                                              *gltf;
  cgltf_result                                             result;
  cgltf_options                                            gltf_options;
  crude_allocator_container                                gltf_allocator_container;

  gltf_allocator_container = crude_heap_allocator_pack( gltf_allocator );

  gltf_options = CRUDE_COMPOUNT_EMPTY( cgltf_options );
  gltf_options.memory.alloc_func = gltf_allocator_container.allocate;
  gltf_options.memory.free_func = gltf_allocator_container.deallocate;
  gltf_options.memory.user_data = gltf_allocator_container.ctx;

  gltf = NULL;
  result = cgltf_parse_file( &gltf_options, gltf_path, &gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_load_buffers( &gltf_options, gltf, gltf_path );
  if ( result != cgltf_result_success )
  {
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, "Failed to load buffers from gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_validate( gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_ASSERT( false );
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to validate gltf file: %s", gltf_path );
    return NULL;
  }

  return gltf;
}

static void
crude_gfx_model_renderer_resources_manager_gltf_load_images_
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
)
{
  for ( uint32 image_index = 0; image_index < gltf->images_count; ++image_index )
  {
    crude_gfx_texture_handle                               texture_handle;
    crude_gfx_texture_creation                             texture_creation;
    cgltf_image const                                     *image;
    char                                                  *image_full_filename;
    int                                                    comp, width, height;
    
    image = &gltf->images[ image_index ];
    image_full_filename = crude_string_buffer_append_use_f( temporary_string_buffer, "%s%s", gltf_directory, image->uri );
    stbi_info( image_full_filename, &width, &height, &comp );
    
    CRUDE_ASSERT( ( width > 0 ) && ( height > 0 ) );

    texture_creation = crude_gfx_texture_creation_empty();
    texture_creation.initial_data = NULL;
    texture_creation.width = width;
    texture_creation.height = height;
    texture_creation.depth = 1u;
    texture_creation.flags = 0u;
    texture_creation.format = VK_FORMAT_R8G8B8A8_UNORM;
    texture_creation.type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D;
    texture_creation.name = image_full_filename;
    texture_creation.subresource.mip_level_count = log2( CRUDE_MAX( width, height ) ) + 1;

    texture_handle = crude_gfx_create_texture( manager->gpu, &texture_creation );
    CRUDE_ARRAY_PUSH( manager->images, texture_handle );
    crude_gfx_asynchronous_loader_request_texture_data( manager->async_loader, image_full_filename, texture_handle );
    crude_string_buffer_clear( temporary_string_buffer );
  }
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_samplers_
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
)
{
  for ( uint32 sampler_index = 0; sampler_index < gltf->samplers_count; ++sampler_index )
  {
    crude_gfx_sampler_handle                               sampler_handle;
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
    
    sampler_handle = crude_gfx_create_sampler( manager->gpu, &creation );
    CRUDE_ARRAY_PUSH( manager->samplers, sampler_handle );
  }
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_textures_
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
  _In_ cgltf_data                                         *gltf
)
{
  for ( uint32 texture_index = 0; texture_index < gltf->textures_count; ++texture_index )
  {
    uint32 sampler_index = cgltf_sampler_index( gltf, gltf->textures[ texture_index ].sampler );
    uint32 image_index = cgltf_image_index( gltf, gltf->textures[ texture_index ].image );
    crude_gfx_link_texture_sampler( manager->gpu, manager->images[ image_index ], manager->samplers[ sampler_index ] );
  }
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_buffers_
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
)
{
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  for ( uint32 buffer_index = 0; buffer_index < gltf->buffers_count; ++buffer_index )
  {
    cgltf_buffer const                                    *buffer;
    crude_gfx_buffer_creation                              buffer_creation;
    crude_gfx_renderer_buffer                             *buffer_resource;
    uint8                                                 *buffer_data;
    char const                                            *buffer_name;

    buffer = &gltf->buffers[ buffer_index ];
  
    buffer_data = ( uint8* )buffer->data;
  
    if ( buffer->name == NULL )
    {
      buffer_name = crude_string_buffer_append_use_f( temporary_string_buffer, "scene_renderer_buffer_gpu_%i", buffer_index );
    }
    else
    {
      buffer_name = buffer->name;
    }

    buffer_creation = crude_gfx_buffer_creation_empty();
    buffer_creation.initial_data = buffer_data;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = buffer->size;
    buffer_creation.type_flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_creation.name = buffer_name;
    buffer_resource = crude_gfx_renderer_create_buffer( scene_renderer->renderer, &buffer_creation );

    CRUDE_ARRAY_PUSH( scene_renderer->buffers, *buffer_resource );
  }
#else
  for ( uint32 buffer_index = 0; buffer_index < gltf->buffers_count; ++buffer_index )
  {
    cgltf_buffer const                                    *buffer;
    crude_gfx_buffer_creation                              cpu_buffer_creation;
    crude_gfx_buffer_creation                              gpu_buffer_creation;
    crude_gfx_buffer_handle                                cpu_buffer;
    crude_gfx_buffer_handle                                gpu_buffer;
    uint8                                                 *buffer_data;
    char const                                            *cpu_buffer_name;
    char const                                            *gpu_buffer_name;

    buffer = &gltf->buffers[ buffer_index ];
  
    buffer_data = ( uint8* )buffer->data;
  
    if ( buffer->name == NULL )
    {
      cpu_buffer_name = "manager_buffer_cpu";
      gpu_buffer_name = crude_string_buffer_append_use_f( temporary_string_buffer, "manager_buffer_gpu_%i", buffer_index );
    }
    else
    {
      cpu_buffer_name = gpu_buffer_name = buffer->name;
    }

    cpu_buffer_creation = crude_gfx_buffer_creation_empty();
    cpu_buffer_creation.initial_data = buffer_data;
    cpu_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    cpu_buffer_creation.size = buffer->size;
    cpu_buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    cpu_buffer_creation.name = cpu_buffer_name;
    cpu_buffer = crude_gfx_create_buffer( manager->gpu, &cpu_buffer_creation );

    gpu_buffer_creation = crude_gfx_buffer_creation_empty();
    gpu_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    gpu_buffer_creation.size = buffer->size;
    gpu_buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    gpu_buffer_creation.name = gpu_buffer_name;
    gpu_buffer_creation.device_only = true;
    gpu_buffer = crude_gfx_create_buffer( manager->gpu, &gpu_buffer_creation );
    CRUDE_ARRAY_PUSH( manager->buffers, gpu_buffer );

    crude_gfx_asynchronous_loader_request_buffer_copy( manager->async_loader, cpu_buffer, gpu_buffer );

    crude_string_buffer_clear( temporary_string_buffer );
  }
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_meshes_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ uint32                                            **gltf_mesh_index_to_mesh_primitive_index,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory,
  _Out_ crude_gfx_mesh_cpu                               **meshes,
  _In_ uint32                                              buffers_offset,
  _In_ uint32                                              images_offset,
  _In_ uint32                                              samplers_offset
)
{
  crude_gfx_mesh_draw_gpu                                 *meshes_draws;
  XMFLOAT4                                                *meshes_bounds;
  crude_gfx_buffer                                        *old_buffer_gpu;
  crude_gfx_buffer_creation                                buffer_creation;
  crude_gfx_buffer_handle                                  buffer_cpu;
  uint64                                                   temporary_allocator_marker, meshes_count;

  temporary_allocator_marker = crude_stack_allocator_get_marker( manager->temporary_allocator )

  CRUDE_ARRAY_SET_LENGTH( *gltf_mesh_index_to_mesh_primitive_index, gltf->meshes_count );
  
  meshes_count = 0;
  for ( uint32 mesh_index = 0; mesh_index < gltf->meshes_count; ++mesh_index )
  { 
    cgltf_mesh *mesh = &gltf->meshes[ mesh_index ];
    (*gltf_mesh_index_to_mesh_primitive_index)[ mesh_index ] = meshes_count;

    for ( uint32 primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index )
    {
      ++meshes_count;
    }
  }

  CRUDE_ARRAY_SET_LENGTH( *meshes, meshes_count );
  
  for ( uint32 mesh_index = 0; mesh_index < gltf->meshes_count; ++mesh_index )
  {
    cgltf_mesh *mesh = &gltf->meshes[ mesh_index ];

    for ( uint32 primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index )
    {
      crude_gfx_mesh_cpu                                   mesh_draw;
      cgltf_primitive                                     *mesh_primitive;
      cgltf_accessor                                      *indices_accessor;
      cgltf_buffer_view                                   *indices_buffer_view;
      crude_gfx_buffer_handle                              indices_buffer_gpu;
      XMVECTOR                                             bounding_center;
      float32                                              bounding_radius;
      uint32                                               flags;
      bool                                                 material_transparent;
      
      mesh_primitive = &mesh->primitives[ primitive_index ];
      mesh_draw = CRUDE_COMPOUNT_EMPTY( crude_gfx_mesh_cpu );
      
      flags = 0;

      for ( uint32 i = 0; i < mesh_primitive->attributes_count; ++i )
      {
        uint32 buffer_offset = mesh_primitive->attributes[ i ].data->offset + mesh_primitive->attributes[ i ].data->buffer_view->offset;

        // !TODO REMOVE
        crude_gfx_buffer_handle buffer_gpu = manager->buffers[ buffers_offset + cgltf_buffer_index( gltf, mesh_primitive->attributes[ i ].data->buffer_view->buffer ) ];
        switch ( mesh_primitive->attributes[ i ].type )
        {
        case cgltf_attribute_type_position:
        {
          XMVECTOR                                         position_max;
          XMVECTOR                                         position_min;
          
          mesh_draw.position_buffer = buffer_gpu;
          mesh_draw.position_offset = buffer_offset;

          CRUDE_ASSERT( sizeof( cgltf_float[4] ) == sizeof( XMFLOAT4 ) );
          CRUDE_ASSERT( mesh_primitive->attributes[ i ].data->has_max && mesh_primitive->attributes[ i ].data->has_min );
          
          position_max = XMLoadFloat4( CRUDE_REINTERPRET_CAST( XMFLOAT4 const*, mesh_primitive->attributes[ i ].data->min ) );
          position_min = XMLoadFloat4( CRUDE_REINTERPRET_CAST( XMFLOAT4 const*, mesh_primitive->attributes[ i ].data->max ) );

          bounding_center = XMVectorAdd( position_min, position_min );
          bounding_center = XMVectorScale( bounding_center, 0.5f );
          bounding_radius = XMVectorGetX( XMVectorMax( XMVector3Length( position_max - bounding_center ), XMVector3Length( position_min - bounding_center ) ) );
          break;
        }
        case cgltf_attribute_type_tangent:
        {
          mesh_draw.tangent_buffer = buffer_gpu;
          mesh_draw.tangent_offset = buffer_offset;
          flags |= CRUDE_GFX_MESH_DRAW_FLAGS_HAS_TANGENTS;
          break;
        }
        case cgltf_attribute_type_normal:
        {
          mesh_draw.normal_buffer = buffer_gpu;
          mesh_draw.normal_offset = buffer_offset;
          flags |= CRUDE_GFX_MESH_DRAW_FLAGS_HAS_NORMAL;
          break;
        }
        case cgltf_attribute_type_texcoord:
        {
          mesh_draw.texcoord_buffer = buffer_gpu;
          mesh_draw.texcoord_offset = buffer_offset;
          break;
        }
        }
      }
      
      indices_accessor = mesh_primitive->indices;
      indices_buffer_view = indices_accessor->buffer_view;
      
      indices_buffer_gpu = manager->buffers[ buffers_offset + cgltf_buffer_index( gltf, indices_accessor->buffer_view->buffer ) ];

      material_transparent = crude_gfx_model_renderer_resources_manager_gltf_create_mesh_material_( manager, gltf, mesh_primitive->material, &mesh_draw, images_offset, samplers_offset );
      
      mesh_draw.index_buffer = indices_buffer_gpu;
      mesh_draw.index_offset = indices_accessor->offset + indices_accessor->buffer_view->offset;
      mesh_draw.primitive_count = indices_accessor->count;
      mesh_draw.gpu_mesh_index = CRUDE_ARRAY_LENGTH( *meshes );

      mesh_draw.bounding_sphere.x = XMVectorGetX( bounding_center );
      mesh_draw.bounding_sphere.y = XMVectorGetY( bounding_center );
      mesh_draw.bounding_sphere.z = XMVectorGetZ( bounding_center );
      mesh_draw.bounding_sphere.w = bounding_radius;

      mesh_draw.flags = flags;

      CRUDE_ARRAY_PUSH( *meshes, mesh_draw );
    }
  }

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( meshes_draws, meshes_count, crude_stack_allocator_pack( manager->temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( meshes_bounds, meshes_count, crude_stack_allocator_pack( manager->temporary_allocator ) );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( meshes ); ++i )
  {
    crude_gfx_mesh_cpu_to_mesh_draw_gpu( manager->gpu, &(*meshes)[ i ], &meshes_draws[ i ] );
    meshes_bounds[ i ] = (*meshes)[ i ].bounding_sphere;
  }

  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof( crude_gfx_mesh_draw_gpu ) * meshes_count;
  buffer_creation.initial_data = meshes_draws;
  buffer_cpu = crude_gfx_create_buffer( manager->gpu, &buffer_creation );
  
  old_buffer_gpu = crude_gfx_access_buffer( manager->gpu, manager->meshlets_triangles_indices_sb );

  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = old_buffer_gpu->size + sizeof( crude_gfx_mesh_draw_gpu ) * meshes_count;
  buffer_creation.device_only = true;
  buffer_creation.name = "meshes_draws_sb";
  manager->meshes_draws_sb = crude_gfx_create_buffer( manager->gpu, &buffer_creation );
  
  crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy( manager->async_loader, buffer_cpu, manager->meshes_draws_sb, old_buffer_gpu->handle );

  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof( XMFLOAT4 ) * meshes_count;
  buffer_creation.initial_data = meshes_bounds;
  buffer_cpu = crude_gfx_create_buffer( manager->gpu, &buffer_creation );
  
  old_buffer_gpu = crude_gfx_access_buffer( manager->gpu, manager->meshlets_triangles_indices_sb );

  buffer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_buffer_creation );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = old_buffer_gpu->size + sizeof( XMFLOAT4 ) * meshes_count;
  buffer_creation.device_only = true;
  buffer_creation.name = "meshes_bounds_sb";
  manager->meshes_bounds_sb = crude_gfx_create_buffer( manager->gpu, &buffer_creation );
    
  crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy( manager->async_loader, buffer_cpu, manager->meshes_bounds_sb, old_buffer_gpu->handle );

  crude_stack_allocator_free_marker( manager->temporary_allocator, temporary_allocator_marker );
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlets_
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
  _In_ cgltf_data                                         *gltf,
  _Out_ crude_gfx_mesh_cpu                                *meshes,
  _In_ uint64                                              meshes_offset
)
{
  crude_gfx_meshlet_gpu                                   *meshlets;
  crude_gfx_meshlet_vertex_gpu                            *meshlets_vertices;
  uint32                                                  *meshlets_vertices_indices;
  uint8                                                   *meshlets_triangles_indices;
  crude_gfx_buffer                                        *old_buffer_gpu;
  uint32                                                   mesh_index;
  crude_gfx_buffer_creation                                buffer_creation;
  crude_gfx_buffer_handle                                  buffer_cpu;
  
  mesh_index = meshes_offset;
  for ( uint32 i = 0; i < gltf->meshes_count; ++i )
  {
    cgltf_mesh *mesh = &gltf->meshes[ i ];
    for ( uint32 primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index )
    {
      cgltf_primitive                                     *mesh_primitive;
      uint32                                              *local_indices;
      meshopt_Meshlet                                     *local_meshlets;
      size_t                                               local_max_meshlets, local_meshletes_count;
      uint32                                               temporary_allocator_marker;
      uint32                                               vertices_offset, meshlets_offset;

      temporary_allocator_marker = crude_stack_allocator_get_marker( manager->temporary_allocator );
      
      mesh_primitive = &mesh->primitives[ primitive_index ];

      /* Load meshlets data*/
      CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( local_indices, 0u, crude_stack_allocator_pack( manager->temporary_allocator ) );
      vertices_offset = CRUDE_ARRAY_LENGTH( meshlets_vertices );
      crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_vertices_( mesh_primitive, &meshlets_vertices );
      crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_indices_( mesh_primitive, &local_indices, vertices_offset );
      
      /* Build meshlets*/
      local_max_meshlets = meshopt_buildMeshletsBound( CRUDE_ARRAY_LENGTH( local_indices ), CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_VERTICES, CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_TRIANGLES );
      
      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( local_meshlets, local_max_meshlets, crude_stack_allocator_pack( manager->temporary_allocator ) );

      /* Increase capacity to have acces to the previous offset, than set lenght based on the last meshlet */
      CRUDE_ARRAY_SET_CAPACITY( meshlets_vertices_indices, CRUDE_ARRAY_LENGTH( meshlets_vertices_indices ) + local_max_meshlets * CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_VERTICES );
      CRUDE_ARRAY_SET_CAPACITY( meshlets_triangles_indices, CRUDE_ARRAY_LENGTH( meshlets_triangles_indices ) + local_max_meshlets * CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_TRIANGLES * 3 );
      uint32 length = CRUDE_ARRAY_LENGTH( meshlets_vertices_indices);
      local_meshletes_count = meshopt_buildMeshlets(
        local_meshlets,
        meshlets_vertices_indices + CRUDE_ARRAY_LENGTH( meshlets_vertices_indices ),
        meshlets_triangles_indices + CRUDE_ARRAY_LENGTH( meshlets_triangles_indices ),
        local_indices, CRUDE_ARRAY_LENGTH( local_indices ), 
        &meshlets_vertices[ 0 ].position.x, CRUDE_ARRAY_LENGTH( meshlets_vertices ), sizeof( crude_gfx_meshlet_vertex_gpu ), 
        CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_VERTICES, CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_TRIANGLES, CRUDE_GRAPHICS_CONSTANT_MESHLET_CONE_WEIGHT
      );
      
      meshlets_offset = CRUDE_ARRAY_LENGTH( meshlets );
      CRUDE_ARRAY_SET_CAPACITY( meshlets, meshlets_offset + local_meshletes_count );

      for ( uint32 meshlet_index = 0; meshlet_index < local_meshletes_count; ++meshlet_index )
      {
        meshopt_Meshlet const *local_meshlet = &local_meshlets[ meshlet_index ];

        CRUDE_ASSERT( local_meshlet->vertex_count <= CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_VERTICES );
        CRUDE_ASSERT( local_meshlet->triangle_count <= CRUDE_GRAPHICS_CONSTANT_MESHLET_MAX_TRIANGLES );

        meshopt_optimizeMeshlet(
          meshlets_vertices_indices + CRUDE_ARRAY_LENGTH( meshlets_vertices_indices ) + local_meshlet->vertex_offset,
          meshlets_triangles_indices + CRUDE_ARRAY_LENGTH( meshlets_triangles_indices ) + local_meshlet->triangle_offset,
          local_meshlet->triangle_count, local_meshlet->vertex_count
        );
      }

      for ( uint32 meshlet_index = 0; meshlet_index < local_meshletes_count; ++meshlet_index )
      {
        meshopt_Meshlet const *local_meshlet = &local_meshlets[ meshlet_index ];

        meshopt_Bounds meshlet_bounds = meshopt_computeMeshletBounds(
          meshlets_vertices_indices + CRUDE_ARRAY_LENGTH( meshlets_vertices_indices ) + local_meshlet->vertex_offset,
          meshlets_triangles_indices + CRUDE_ARRAY_LENGTH( meshlets_triangles_indices ) + local_meshlet->triangle_offset,
          local_meshlet->triangle_count, &meshlets_vertices[ 0 ].position.x, CRUDE_ARRAY_LENGTH( meshlets_vertices ), sizeof( crude_gfx_meshlet_vertex_gpu )
        );;

        crude_gfx_meshlet_gpu new_meshlet = CRUDE_COMPOUNT_EMPTY( crude_gfx_meshlet_gpu );
        new_meshlet.vertices_offset = CRUDE_ARRAY_LENGTH( meshlets_vertices_indices ) + local_meshlet->vertex_offset;
        new_meshlet.triangles_offset = CRUDE_ARRAY_LENGTH( meshlets_triangles_indices ) + local_meshlet->triangle_offset;
        new_meshlet.vertices_count = local_meshlet->vertex_count;
        new_meshlet.triangles_count = local_meshlet->triangle_count;
        new_meshlet.mesh_index = mesh_index;

        new_meshlet.center = CRUDE_COMPOUNT( XMFLOAT3, { meshlet_bounds.center[ 0 ], meshlet_bounds.center[ 1 ], meshlet_bounds.center[ 2 ] } );
        new_meshlet.radius = meshlet_bounds.radius;
        
        new_meshlet.cone_axis[ 0 ] = meshlet_bounds.cone_axis_s8[ 0 ];
        new_meshlet.cone_axis[ 1 ] = meshlet_bounds.cone_axis_s8[ 1 ];
        new_meshlet.cone_axis[ 2 ] = meshlet_bounds.cone_axis_s8[ 2 ];

        new_meshlet.cone_cutoff = meshlet_bounds.cone_cutoff_s8;

        CRUDE_ARRAY_PUSH( meshlets, new_meshlet );
      }
      
      crude_gfx_meshlet_gpu const *last_meshlet = &CRUDE_ARRAY_BACK( meshlets );
      CRUDE_ARRAY_SET_LENGTH( meshlets_vertices_indices, last_meshlet->vertices_offset + last_meshlet->vertices_count );
      CRUDE_ARRAY_SET_LENGTH( meshlets_triangles_indices, last_meshlet->triangles_offset + 3u * last_meshlet->triangles_count );

      crude_stack_allocator_free_marker( manager->temporary_allocator, temporary_allocator_marker );
      
      CRUDE_ASSERT( false );
      meshes[ mesh_index ].meshlets_count = local_meshletes_count;
      meshes[ mesh_index ].meshlets_offset = meshlets_offset;
      ++mesh_index;
    }
  }

  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof*( meshlets_triangles_indices ) * CRUDE_ARRAY_LENGTH( meshlets_triangles_indices );
  buffer_creation.initial_data = meshlets_triangles_indices;
  buffer_cpu = crude_gfx_create_buffer( manager->gpu, &buffer_creation );

  old_buffer_gpu = crude_gfx_access_buffer( manager->gpu, manager->meshlets_triangles_indices_sb );
  
  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = old_buffer_gpu->size + sizeof*( meshlets_triangles_indices ) * CRUDE_ARRAY_LENGTH( meshlets_triangles_indices );
  buffer_creation.device_only = true;
  buffer_creation.name = "meshlets_triangles_indices_sb";
  manager->meshlets_triangles_indices_sb = crude_gfx_create_buffer( manager->gpu, &buffer_creation );

  crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy( manager->async_loader, buffer_cpu, manager->meshlets_triangles_indices_sb, old_buffer_gpu->handle );

  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.initial_data = meshlets;
  buffer_creation.size = sizeof*( meshlets ) * CRUDE_ARRAY_LENGTH( meshlets );
  buffer_cpu = crude_gfx_create_buffer( manager->gpu, &buffer_creation );
  
  old_buffer_gpu = crude_gfx_access_buffer( manager->gpu, manager->meshlets_sb );

  buffer_creation = crude_gfx_buffer_creation_empty();
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = old_buffer_gpu->size + sizeof*( meshlets ) * CRUDE_ARRAY_LENGTH( meshlets );
  buffer_creation.name = "meshlet_sb";
  buffer_creation.device_only = true;
  manager->meshlets_sb = crude_gfx_create_buffer( manager->gpu, &buffer_creation );

  crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy( manager->async_loader, buffer_cpu, manager->meshlets_sb, old_buffer_gpu->handle );
    
  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof*( meshlets_vertices ) * CRUDE_ARRAY_LENGTH( meshlets_vertices );
  buffer_creation.initial_data = meshlets_vertices;
  buffer_cpu = crude_gfx_create_buffer( manager->gpu, &buffer_creation );
    
  old_buffer_gpu = crude_gfx_access_buffer( manager->gpu, manager->meshlets_vertices_sb );

  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = old_buffer_gpu->size + sizeof*( meshlets_vertices ) * CRUDE_ARRAY_LENGTH( meshlets_vertices );
  buffer_creation.device_only = true;
  buffer_creation.name = "meshlets_vertices_sb";
  manager->meshlets_vertices_sb = crude_gfx_create_buffer( manager->gpu, &buffer_creation );

  crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy( manager->async_loader, buffer_cpu, manager->meshlets_vertices_sb, old_buffer_gpu->handle );

  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof*( meshlets_vertices_indices ) * CRUDE_ARRAY_LENGTH( meshlets_vertices_indices );
  buffer_creation.initial_data = meshlets_vertices_indices;
  buffer_cpu = crude_gfx_create_buffer( manager->gpu, &buffer_creation );
    
  old_buffer_gpu = crude_gfx_access_buffer( manager->gpu, manager->meshlets_vertices_indices_sb );

  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = old_buffer_gpu->size + sizeof*( meshlets_vertices_indices ) * CRUDE_ARRAY_LENGTH( meshlets_vertices_indices );
  buffer_creation.device_only = true;
  buffer_creation.name = "meshlets_vertices_indices_sb";
  manager->meshlets_vertices_indices_sb = crude_gfx_create_buffer( manager->gpu, &buffer_creation );

  crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy( manager->async_loader, buffer_cpu, manager->meshlets_vertices_indices_sb, old_buffer_gpu->handle );
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_nodes_
(
	_In_ crude_gfx_model_renderer_resources_manager					*manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_entity                                        parent_node,
  _In_ cgltf_node                                        **gltf_nodes,
  _In_ uint32                                              gltf_nodes_count,
  _In_ uint32                                             *gltf_mesh_index_to_mesh_primitive_index
)
{ 
  for ( uint32 i = 0u; i < gltf_nodes_count; ++i )
  {
    crude_entity                                           node;
    crude_transform                                        transform;
    
    node = crude_entity_create_empty( parent_node.world, gltf_nodes[ i ]->name );
    crude_entity_set_parent( node, parent_node );
    
    if ( gltf_nodes[ i ]->has_translation )
    {
      XMStoreFloat3( &transform.translation, XMVectorSet( gltf_nodes[ i ]->translation[ 0 ], gltf_nodes[ i ]->translation[ 1 ], gltf_nodes[ i ]->translation[ 2 ], 1 ));
    }
    else
    {
      XMStoreFloat3( &transform.translation, XMVectorZero( ) );
    }

    if ( gltf_nodes[ i ]->has_scale )
    {
      XMStoreFloat3( &transform.scale, XMVectorSet( gltf_nodes[ i ]->scale[ 0 ], gltf_nodes[ i ]->scale[ 1 ], gltf_nodes[ i ]->scale[ 2 ], 1 ));
    }
    else
    {
      XMStoreFloat3( &transform.scale, XMVectorReplicate( 1.f ) );
    }

    if ( gltf_nodes[ i ]->has_rotation )
    {
      CRUDE_ASSERT( false );
    }
    else
    {
      XMStoreFloat4( &transform.rotation, XMQuaternionIdentity( ) );
    }
    
    if ( gltf_nodes[ i ]->has_matrix )
    {
      XMVECTOR                                             decompose_scale;
      XMVECTOR                                             decompose_rotation_quat;
      XMVECTOR                                             decompose_translation;
      XMFLOAT4X4                                           gltf_node_matrix;

      CRUDE_ASSERT( !gltf_nodes[ i ]->has_translation );
      CRUDE_ASSERT( !gltf_nodes[ i ]->has_scale );
      CRUDE_ASSERT( !gltf_nodes[ i ]->has_rotation );
      gltf_node_matrix = XMFLOAT4X4{ gltf_nodes[ i ]->matrix };
      XMMatrixDecompose( &decompose_scale, &decompose_rotation_quat, &decompose_translation, XMLoadFloat4x4( &gltf_node_matrix ) );
      XMStoreFloat3( &transform.translation, decompose_translation );
      XMStoreFloat4( &transform.rotation, decompose_rotation_quat );
      XMStoreFloat3( &transform.scale, decompose_scale );
    }

    CRUDE_ENTITY_SET_COMPONENT( node, crude_transform, {
      transform.translation, transform.rotation, transform.scale
    } );
    
    if ( gltf_nodes[ i ]->mesh )
    {
      uint32 mesh_index_offset = gltf_mesh_index_to_mesh_primitive_index[ cgltf_mesh_index( gltf, gltf_nodes[ i ]->mesh ) ];
      for ( uint32 pi = 0; pi < gltf_nodes[ i ]->mesh->primitives_count; ++pi )
      {
        crude_gfx_mesh_instance_cpu mesh_instance;
        mesh_instance.mesh_gpu_index = mesh_index_offset + pi;
        mesh_instance.node = node;
        CRUDE_ARRAY_PUSH( model_renderer_resources->meshes_instances, mesh_instance );
      }
    }

    crude_gfx_model_renderer_resources_manager_gltf_load_nodes_( manager, model_renderer_resources, gltf, node, gltf_nodes[ i ]->children, gltf_nodes[ i ]->children_count, gltf_mesh_index_to_mesh_primitive_index );
  }
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_vertices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ crude_gfx_meshlet_vertex_gpu                      **vertices
)
{
  XMFLOAT4                                                *primitive_tangents;
  XMFLOAT3                                                *primitive_positions;
  XMFLOAT3                                                *primitive_normals;
  XMFLOAT2                                                *primitive_texcoords;
  uint32                                                   meshlet_vertices_count;
  
  primitive_tangents = NULL;
  primitive_positions = primitive_normals = NULL;
  primitive_texcoords = NULL;

  meshlet_vertices_count = primitive->attributes[ 0 ].data->count;
  
  for ( uint32 i = 0; i < primitive->attributes_count; ++i )
  {
    cgltf_attribute *attribute = &primitive->attributes[ i ];
    CRUDE_ASSERT( meshlet_vertices_count == attribute->data->count );

    uint8 *attribute_data = CRUDE_STATIC_CAST( uint8*, attribute->data->buffer_view->buffer->data ) + attribute->data->buffer_view->offset + attribute->data->offset;
    switch ( attribute->type )
    {
    case cgltf_attribute_type_position:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec3 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( XMFLOAT3 ) );
      primitive_positions = CRUDE_CAST( XMFLOAT3*, attribute_data );
      break;
    }
    case cgltf_attribute_type_tangent:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec4 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( XMFLOAT4 ) );
      primitive_tangents = CRUDE_CAST( XMFLOAT4*, attribute_data );
      break;
    }
    case cgltf_attribute_type_normal:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec3 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( XMFLOAT3 ) );
      primitive_normals = CRUDE_CAST( XMFLOAT3*, attribute_data );
      break;
    }
    case cgltf_attribute_type_texcoord:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec2 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( XMFLOAT2 ) );
      primitive_texcoords = CRUDE_CAST( XMFLOAT2*, attribute_data );
      break;
    }
    }
  }

  uint32 old_length = CRUDE_ARRAY_LENGTH( *vertices );
  uint32 new_cap = CRUDE_ARRAY_LENGTH( *vertices ) + meshlet_vertices_count;
  CRUDE_ARRAY_SET_CAPACITY( *vertices, CRUDE_ARRAY_LENGTH( *vertices ) + meshlet_vertices_count );
  for ( uint32 i = 0; i < meshlet_vertices_count; ++i )
  {
    CRUDE_ASSERT( primitive_positions );
    
    crude_gfx_meshlet_vertex_gpu new_meshlet_vertex = CRUDE_COMPOUNT_EMPTY( crude_gfx_meshlet_vertex_gpu );

    new_meshlet_vertex.position.x = primitive_positions[ i ].x;
    new_meshlet_vertex.position.y = primitive_positions[ i ].y;
    new_meshlet_vertex.position.z = primitive_positions[ i ].z;

    if ( primitive_normals )
    {
      new_meshlet_vertex.normal[ 0 ] = ( primitive_normals[ i ].x + 1.0f ) * 127.0f;
      new_meshlet_vertex.normal[ 1 ] = ( primitive_normals[ i ].y + 1.0f ) * 127.0f;
      new_meshlet_vertex.normal[ 2 ] = ( primitive_normals[ i ].z + 1.0f ) * 127.0f;
    }

    if ( primitive_tangents  )
    {
      new_meshlet_vertex.tangent[ 0 ] = ( primitive_tangents[ i ].x + 1.0f ) * 127.0f;
      new_meshlet_vertex.tangent[ 1 ] = ( primitive_tangents[ i ].y + 1.0f ) * 127.0f;
      new_meshlet_vertex.tangent[ 2 ] = ( primitive_tangents[ i ].z + 1.0f ) * 127.0f;
      new_meshlet_vertex.tangent[ 3 ] = ( primitive_tangents[ i ].w + 1.0f ) * 127.0f;
    }

    if ( primitive_texcoords )
    {
      new_meshlet_vertex.texcoords[ 0 ] = meshopt_quantizeHalf( primitive_texcoords[ i ].x );
      new_meshlet_vertex.texcoords[ 1 ] = meshopt_quantizeHalf( primitive_texcoords[ i ].y );
    }
    CRUDE_ARRAY_PUSH( *vertices, new_meshlet_vertex );
  }
}

static void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_indices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ uint32                                            **indices,
  _In_ uint32                                              vertices_offset
)
{
  uint32                                                   meshlet_vertices_indices_count;
  uint8                                                   *buffer_data;

  meshlet_vertices_indices_count = primitive->indices->count;
  buffer_data = CRUDE_CAST( uint8*, primitive->indices->buffer_view->buffer->data ) + primitive->indices->buffer_view->offset + primitive->indices->offset;
  CRUDE_ARRAY_SET_LENGTH( *indices, meshlet_vertices_indices_count );
  
  CRUDE_ASSERT( primitive->indices->type == cgltf_type_scalar );
  CRUDE_ASSERT( primitive->indices->component_type == cgltf_component_type_r_16u ); // change ray tracing index property in geometry 

  if ( primitive->indices->component_type == cgltf_component_type_r_16u )
  {
    uint16 *primitive_indices = CRUDE_CAST( uint16*, buffer_data );
    for ( uint32 i = 0; i < meshlet_vertices_indices_count; ++i )
    {
      ( *indices )[ i ] = primitive_indices[ i ] + vertices_offset;
    }
  }
  else if ( primitive->indices->component_type == cgltf_component_type_r_32u )
  {
    uint32 *primitive_indices = CRUDE_CAST( uint32*, buffer_data );
    for ( uint32 i = 0; i < meshlet_vertices_indices_count; ++i )
    {
      ( *indices )[ i ] = primitive_indices[ i ] + vertices_offset;
    }
  }
  else
  {
    CRUDE_ASSERT( false );
  }
}
void
create_bottom_level_acceleration_structure_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ uint64                                              meshes_offset
)
{
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_cmd_buffer                                    *cmd;
  crude_gfx_buffer_handle                                 *blas_scratch_buffers_handle;
  VkAccelerationStructureBuildRangeInfoKHR               **vk_acceleration_structure_build_range_infos;
  VkAccelerationStructureBuildGeometryInfoKHR             *vk_acceleration_build_geometry_infos;
  VkAccelerationStructureGeometryKHR                      *vk_geometries;
  VkTransformMatrixKHR                                     vk_transform_matrix;
  crude_gfx_buffer_handle                                  scratch_transform_buffer;
  crude_gfx_buffer_creation                                buffer_creation;
  uint32                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );
  
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( vk_acceleration_structure_build_range_infos, CRUDE_ARRAY_LENGTH( scene_renderer->meshes ) - meshes_offset, crude_stack_allocator_pack( temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( vk_acceleration_build_geometry_infos, CRUDE_ARRAY_LENGTH( scene_renderer->meshes ) - meshes_offset, crude_stack_allocator_pack( temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( blas_scratch_buffers_handle, CRUDE_ARRAY_LENGTH( scene_renderer->meshes ) - meshes_offset, crude_stack_allocator_pack( temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_geometries, CRUDE_ARRAY_LENGTH( scene_renderer->meshes ) - meshes_offset, crude_stack_allocator_pack( temporary_allocator ) );

  vk_transform_matrix = CRUDE_COMPOUNT( VkTransformMatrixKHR, {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f
  } );
  
  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = sizeof( vk_transform_matrix );
  buffer_creation.initial_data = &vk_transform_matrix;
  buffer_creation.name = "scratch_transform_buffer";
  scratch_transform_buffer = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes ) - meshes_offset; ++i )
  {
    crude_gfx_buffer                                       *blas_buffer;
    crude_gfx_mesh_cpu                                     *mesh;
    crude_gfx_buffer_handle                                 blas_scratch_buffer_handle;
    VkAccelerationStructureBuildRangeInfoKHR               *vk_acceleration_structure_build_range_info;
    VkAccelerationStructureBuildGeometryInfoKHR             vk_acceleration_build_geometry_info;
    VkAccelerationStructureKHR                              vk_blas;
    VkAccelerationStructureCreateInfoKHR                    vk_acceleration_structure_create_info;
    VkAccelerationStructureBuildSizesInfoKHR                vk_acceleration_structure_build_sizes_info;
    VkTransformMatrixKHR                                    vk_transform; 
    crude_gfx_buffer_handle                                 blas_buffer_handle;
    XMFLOAT4X4                                              node_to_world;
    uint32                                                  vertices_count;
    uint32                                                  max_primitives_count;

    mesh = &scene_renderer->meshes[ i + meshes_offset ];
    vertices_count = mesh->primitive_count / 3;
  
    vk_geometries[ i ] = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureGeometryKHR );
    vk_geometries[ i ].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    vk_geometries[ i ].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    vk_geometries[ i ].flags = crude_gfx_mesh_is_transparent( mesh ) ? 0 : VK_GEOMETRY_OPAQUE_BIT_KHR;
    vk_geometries[ i ].geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    vk_geometries[ i ].geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    vk_geometries[ i ].geometry.triangles.vertexData.deviceAddress = crude_gfx_get_buffer_device_address( scene_renderer->renderer->gpu, mesh->position_buffer ) + mesh->position_offset;
    vk_geometries[ i ].geometry.triangles.vertexStride = sizeof( XMFLOAT3 );
    vk_geometries[ i ].geometry.triangles.maxVertex = vertices_count;
    vk_geometries[ i ].geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
    vk_geometries[ i ].geometry.triangles.indexData.deviceAddress = crude_gfx_get_buffer_device_address( scene_renderer->renderer->gpu, mesh->index_buffer ) + mesh->index_offset;
    vk_geometries[ i ].geometry.triangles.transformData.deviceAddress = crude_gfx_get_buffer_device_address( scene_renderer->renderer->gpu, scratch_transform_buffer );

    vk_acceleration_structure_build_range_info = CRUDE_CAST( VkAccelerationStructureBuildRangeInfoKHR*, crude_stack_allocator_allocate( temporary_allocator, sizeof( VkAccelerationStructureBuildRangeInfoKHR ) ) );
    *vk_acceleration_structure_build_range_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildRangeInfoKHR );
    vk_acceleration_structure_build_range_info->primitiveCount = vertices_count;
    vk_acceleration_structure_build_range_info->primitiveOffset = 0;
    vk_acceleration_structure_build_range_info->transformOffset = 0;

    vk_acceleration_build_geometry_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
    vk_acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    vk_acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    vk_acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    vk_acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    vk_acceleration_build_geometry_info.geometryCount = 1u;
    vk_acceleration_build_geometry_info.pGeometries = &vk_geometries[ i ];

    max_primitives_count = vk_acceleration_structure_build_range_info->primitiveCount;
  
    vk_acceleration_structure_build_sizes_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildSizesInfoKHR );
    vk_acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    scene_renderer->renderer->gpu->vkGetAccelerationStructureBuildSizesKHR( scene_renderer->renderer->gpu->vk_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &vk_acceleration_build_geometry_info, &max_primitives_count, &vk_acceleration_structure_build_sizes_info );

    buffer_creation = crude_gfx_buffer_creation_empty( );
    buffer_creation.type_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = vk_acceleration_structure_build_sizes_info.accelerationStructureSize;
    buffer_creation.device_only = true;
    buffer_creation.name = "blas_buffer";
    blas_buffer_handle = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    blas_buffer = crude_gfx_access_buffer( scene_renderer->renderer->gpu, blas_buffer_handle );

    vk_acceleration_structure_create_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureCreateInfoKHR );
    vk_acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    vk_acceleration_structure_create_info.buffer = blas_buffer->vk_buffer;
    vk_acceleration_structure_create_info.offset = 0;
    vk_acceleration_structure_create_info.size = vk_acceleration_structure_build_sizes_info.accelerationStructureSize;
    vk_acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    CRUDE_GFX_HANDLE_VULKAN_RESULT( scene_renderer->renderer->gpu->vkCreateAccelerationStructureKHR( scene_renderer->renderer->gpu->vk_device, &vk_acceleration_structure_create_info, scene_renderer->renderer->gpu->vk_allocation_callbacks, &vk_blas ), "Can't create acceleration structure" );

    // TODO maybe we can use only one scratch buffer? idk for now
    buffer_creation = crude_gfx_buffer_creation_empty( );
    buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
    buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    buffer_creation.size = vk_acceleration_structure_build_sizes_info.buildScratchSize;
    buffer_creation.device_only = true;
    buffer_creation.name = "blas_scratch_buffer";
    blas_scratch_buffer_handle = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

    vk_acceleration_build_geometry_info.dstAccelerationStructure = vk_blas;
    vk_acceleration_build_geometry_info.scratchData.deviceAddress = crude_gfx_get_buffer_device_address( scene_renderer->renderer->gpu, blas_scratch_buffer_handle );

    CRUDE_ARRAY_PUSH( vk_acceleration_structure_build_range_infos, vk_acceleration_structure_build_range_info );
    CRUDE_ARRAY_PUSH( vk_acceleration_build_geometry_infos, vk_acceleration_build_geometry_info );
    CRUDE_ARRAY_PUSH( blas_scratch_buffers_handle, blas_scratch_buffer_handle );
    CRUDE_ARRAY_PUSH( scene_renderer->vk_blases, vk_blas );
    CRUDE_ARRAY_PUSH( scene_renderer->blases_buffers, blas_buffer_handle );
  }
  
  cmd = crude_gfx_get_primary_cmd( scene_renderer->renderer->gpu, 0, true );
  scene_renderer->renderer->gpu->vkCmdBuildAccelerationStructuresKHR( cmd->vk_cmd_buffer, CRUDE_ARRAY_LENGTH( vk_acceleration_build_geometry_infos ), vk_acceleration_build_geometry_infos, vk_acceleration_structure_build_range_infos );
  crude_gfx_submit_immediate( cmd );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blas_scratch_buffers_handle ); ++i )
  {
    crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, blas_scratch_buffers_handle[ i ] );
  }

  crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scratch_transform_buffer );

  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
}

void
create_top_level_acceleration_structure_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager
)
{
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_buffer                                        *tlas_buffer;
  crude_gfx_cmd_buffer                                    *cmd;
  VkAccelerationStructureInstanceKHR                      *vk_acceleration_structure_instances;
  VkAccelerationStructureBuildRangeInfoKHR                 vk_acceleration_structure_build_range_info;
  VkAccelerationStructureCreateInfoKHR                     vk_acceleration_structure_create_info;
  VkAccelerationStructureBuildSizesInfoKHR                 vk_acceleration_structure_build_sizes_info;
  VkAccelerationStructureBuildGeometryInfoKHR              vk_acceleration_build_geometry_info;
  VkAccelerationStructureGeometryKHR                       vk_acceleration_structure_geometry;
  crude_gfx_buffer_creation                                buffer_creation;
  uint64                                                   temporary_allocator_marker;
  uint32                                                   max_instance_count;

  temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( vk_acceleration_structure_instances, CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ), crude_stack_allocator_pack( temporary_allocator ) );

  CRUDE_ASSERT( CRUDE_ARRAY_LENGTH( scene_renderer->vk_blases ) == CRUDE_ARRAY_LENGTH( scene_renderer->meshes ) );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ); ++i )
  {
    crude_gfx_mesh_instance_cpu                            *mesh_instance;
    VkAccelerationStructureInstanceKHR                     vk_acceleration_structure_instance;
    VkAccelerationStructureDeviceAddressInfoKHR            vk_acceleration_structure_address_info;
    VkDeviceAddress                                        vk_blas_address;
    VkTransformMatrixKHR                                   vk_transform; 
    XMFLOAT4X4                                             node_to_world;
    int64                                                  mesh_index;
    
    mesh_instance = &scene_renderer->meshes_instances[ i ];
    mesh_index = mesh_instance->mesh_cpu_index;

    XMStoreFloat4x4( &node_to_world, crude_transform_node_to_world( mesh_instance->node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( mesh_instance->node, crude_transform ) ) );
    for ( int32 y = 0; y < 3; ++y )
    {
      for ( int32 x = 0; x < 4; ++x )
      {
        vk_transform.matrix[ y ][ x ] = node_to_world.m[ x ][ y ];
      }
    }

    vk_acceleration_structure_address_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureDeviceAddressInfoKHR );
    vk_acceleration_structure_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    vk_acceleration_structure_address_info.accelerationStructure = scene_renderer->vk_blases[ mesh_index ];

    vk_blas_address = scene_renderer->renderer->gpu->vkGetAccelerationStructureDeviceAddressKHR( scene_renderer->renderer->gpu->vk_device, &vk_acceleration_structure_address_info );
    
    vk_acceleration_structure_instance = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureInstanceKHR );
    vk_acceleration_structure_instance.mask = 0xff;
    vk_acceleration_structure_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    vk_acceleration_structure_instance.accelerationStructureReference = vk_blas_address;
    vk_acceleration_structure_instance.transform = vk_transform;

    CRUDE_ARRAY_PUSH( vk_acceleration_structure_instances, vk_acceleration_structure_instance );
  }

  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ) * sizeof( VkAccelerationStructureInstanceKHR );
  buffer_creation.initial_data = vk_acceleration_structure_instances;
  buffer_creation.name = "tlas_instances_buffer";
  scene_renderer->tlas_instances_buffer_handle = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

  max_instance_count = CRUDE_ARRAY_LENGTH( vk_acceleration_structure_instances );
  
  vk_acceleration_structure_geometry = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureGeometryKHR );
  vk_acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  vk_acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  vk_acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  vk_acceleration_structure_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  vk_acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
  vk_acceleration_structure_geometry.geometry.instances.data.deviceAddress = crude_gfx_get_buffer_device_address( scene_renderer->renderer->gpu, scene_renderer->tlas_instances_buffer_handle );

  vk_acceleration_build_geometry_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
  vk_acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  vk_acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  vk_acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  vk_acceleration_build_geometry_info.geometryCount = 1;
  vk_acceleration_build_geometry_info.pGeometries = &vk_acceleration_structure_geometry;

  vk_acceleration_structure_build_sizes_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildSizesInfoKHR );
  vk_acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  scene_renderer->renderer->gpu->vkGetAccelerationStructureBuildSizesKHR( scene_renderer->renderer->gpu->vk_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &vk_acceleration_build_geometry_info, &max_instance_count, &vk_acceleration_structure_build_sizes_info );
  
  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = vk_acceleration_structure_build_sizes_info.accelerationStructureSize;
  buffer_creation.device_only = true;
  buffer_creation.name = "tlas_buffer";
  scene_renderer->tlas_buffer = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

  tlas_buffer = crude_gfx_access_buffer( scene_renderer->renderer->gpu, scene_renderer->tlas_buffer );
  
  vk_acceleration_structure_create_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureCreateInfoKHR );
  vk_acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  vk_acceleration_structure_create_info.buffer = tlas_buffer->vk_buffer;
  vk_acceleration_structure_create_info.size = vk_acceleration_structure_build_sizes_info.accelerationStructureSize;
  vk_acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

  scene_renderer->renderer->gpu->vkCreateAccelerationStructureKHR( scene_renderer->renderer->gpu->vk_device, &vk_acceleration_structure_create_info, scene_renderer->renderer->gpu->vk_allocation_callbacks, &scene_renderer->vk_tlas );

  buffer_creation = crude_gfx_buffer_creation_empty( );
  buffer_creation.type_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
  buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
  buffer_creation.size = vk_acceleration_structure_build_sizes_info.buildScratchSize;
  buffer_creation.device_only = true;
  buffer_creation.name = "tlas_scratch_buffer";
  scene_renderer->tlas_scratch_buffer_handle = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &buffer_creation );

  vk_acceleration_build_geometry_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
  vk_acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  vk_acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  vk_acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  vk_acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  vk_acceleration_build_geometry_info.dstAccelerationStructure = scene_renderer->vk_tlas;
  vk_acceleration_build_geometry_info.geometryCount = 1;
  vk_acceleration_build_geometry_info.pGeometries = &vk_acceleration_structure_geometry;
  vk_acceleration_build_geometry_info.scratchData.deviceAddress = crude_gfx_get_buffer_device_address( scene_renderer->renderer->gpu, scene_renderer->tlas_scratch_buffer_handle );

  vk_acceleration_structure_build_range_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildRangeInfoKHR );
  vk_acceleration_structure_build_range_info.primitiveCount = CRUDE_ARRAY_LENGTH( vk_acceleration_structure_instances );
  vk_acceleration_structure_build_range_info.primitiveOffset = 0;
  vk_acceleration_structure_build_range_info.firstVertex = 0;
  vk_acceleration_structure_build_range_info.transformOffset = 0;

  VkAccelerationStructureBuildRangeInfoKHR *tlas_ranges[ ] = {
    &vk_acceleration_structure_build_range_info
  };
  
  cmd = crude_gfx_get_primary_cmd( scene_renderer->renderer->gpu, 3, true );
  scene_renderer->renderer->gpu->vkCmdBuildAccelerationStructuresKHR( cmd->vk_cmd_buffer, 1, &vk_acceleration_build_geometry_info, tlas_ranges );
  crude_gfx_submit_immediate( cmd );

  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
}

void
update_top_level_acceleration_structure_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager
)
{
#ifdef CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_buffer                                        *tlas_buffer;
  crude_gfx_cmd_buffer                                    *cmd;
  VkAccelerationStructureInstanceKHR                      *vk_acceleration_structure_instances;
  VkAccelerationStructureBuildRangeInfoKHR                 vk_acceleration_structure_build_range_info;
  VkAccelerationStructureBuildSizesInfoKHR                 vk_acceleration_structure_build_sizes_info;
  VkAccelerationStructureBuildGeometryInfoKHR              vk_acceleration_build_geometry_info;
  VkAccelerationStructureGeometryKHR                       vk_acceleration_structure_geometry;
  crude_gfx_buffer_creation                                buffer_creation;
  crude_gfx_map_buffer_parameters                          buffer_map;
  uint64                                                   temporary_allocator_marker;
  uint32                                                   max_instance_count;

  temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( vk_acceleration_structure_instances, CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ), crude_stack_allocator_pack( temporary_allocator ) );

  CRUDE_ASSERT( CRUDE_ARRAY_LENGTH( scene_renderer->vk_blases ) == CRUDE_ARRAY_LENGTH( scene_renderer->meshes ) );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes_instances ); ++i )
  {
    crude_gfx_mesh_instance_cpu                            *mesh_instance;
    VkAccelerationStructureInstanceKHR                     vk_acceleration_structure_instance;
    VkAccelerationStructureDeviceAddressInfoKHR            vk_acceleration_structure_address_info;
    VkDeviceAddress                                        vk_blas_address;
    VkTransformMatrixKHR                                   vk_transform; 
    XMFLOAT4X4                                             node_to_world;
    int64                                                  mesh_index;
    
    mesh_instance = &scene_renderer->meshes_instances[ i ];
    mesh_index = mesh_instance->mesh_cpu_index;

    XMStoreFloat4x4( &node_to_world, crude_transform_node_to_world( mesh_instance->node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( mesh_instance->node, crude_transform ) ) );
    for ( int32 y = 0; y < 3; ++y )
    {
      for ( int32 x = 0; x < 4; ++x )
      {
        vk_transform.matrix[ y ][ x ] = node_to_world.m[ x ][ y ];
      }
    }

    vk_acceleration_structure_address_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureDeviceAddressInfoKHR );
    vk_acceleration_structure_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    vk_acceleration_structure_address_info.accelerationStructure = scene_renderer->vk_blases[ mesh_index ];

    vk_blas_address = scene_renderer->renderer->gpu->vkGetAccelerationStructureDeviceAddressKHR( scene_renderer->renderer->gpu->vk_device, &vk_acceleration_structure_address_info );
    
    vk_acceleration_structure_instance = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureInstanceKHR );
    vk_acceleration_structure_instance.mask = 0xff;
    vk_acceleration_structure_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    vk_acceleration_structure_instance.accelerationStructureReference = vk_blas_address;
    vk_acceleration_structure_instance.transform = vk_transform;

    CRUDE_ARRAY_PUSH( vk_acceleration_structure_instances, vk_acceleration_structure_instance );
  }

  buffer_map = CRUDE_COMPOUNT_EMPTY( crude_gfx_map_buffer_parameters );
  buffer_map.buffer = scene_renderer->tlas_instances_buffer_handle;
  void *tlas_instances_buffer_mapped_data = crude_gfx_map_buffer( scene_renderer->renderer->gpu, &buffer_map );
  if ( tlas_instances_buffer_mapped_data )
  {
    crude_memory_copy( tlas_instances_buffer_mapped_data, vk_acceleration_structure_instances, CRUDE_ARRAY_LENGTH( vk_acceleration_structure_instances ) * sizeof( VkAccelerationStructureInstanceKHR ) );
    crude_gfx_unmap_buffer( scene_renderer->renderer->gpu, scene_renderer->tlas_instances_buffer_handle );
  }
  

  max_instance_count = CRUDE_ARRAY_LENGTH( vk_acceleration_structure_instances );
  
  vk_acceleration_structure_geometry = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureGeometryKHR );
  vk_acceleration_structure_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  vk_acceleration_structure_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  vk_acceleration_structure_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  vk_acceleration_structure_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  vk_acceleration_structure_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
  vk_acceleration_structure_geometry.geometry.instances.data.deviceAddress = crude_gfx_get_buffer_device_address( scene_renderer->renderer->gpu, scene_renderer->tlas_instances_buffer_handle );

  vk_acceleration_build_geometry_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
  vk_acceleration_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  vk_acceleration_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  vk_acceleration_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
  vk_acceleration_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
  vk_acceleration_build_geometry_info.dstAccelerationStructure = scene_renderer->vk_tlas;
  vk_acceleration_build_geometry_info.srcAccelerationStructure = scene_renderer->vk_tlas;
  vk_acceleration_build_geometry_info.geometryCount = 1;
  vk_acceleration_build_geometry_info.pGeometries = &vk_acceleration_structure_geometry;
  vk_acceleration_build_geometry_info.scratchData.deviceAddress = crude_gfx_get_buffer_device_address( scene_renderer->renderer->gpu, scene_renderer->tlas_scratch_buffer_handle );

  vk_acceleration_structure_build_range_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildRangeInfoKHR );
  vk_acceleration_structure_build_range_info.primitiveCount = CRUDE_ARRAY_LENGTH( vk_acceleration_structure_instances );
  vk_acceleration_structure_build_range_info.primitiveOffset = 0;
  vk_acceleration_structure_build_range_info.firstVertex = 0;
  vk_acceleration_structure_build_range_info.transformOffset = 0;

  VkAccelerationStructureBuildRangeInfoKHR *tlas_ranges[ ] = {
    &vk_acceleration_structure_build_range_info
  };
  
  cmd = crude_gfx_get_primary_cmd( scene_renderer->renderer->gpu, 0, true );
  scene_renderer->renderer->gpu->vkCmdBuildAccelerationStructuresKHR( cmd->vk_cmd_buffer, 1, &vk_acceleration_build_geometry_info, tlas_ranges );
  crude_gfx_submit_immediate( cmd );

  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
}