#include <cgltf.h>
#include <stb_image.h>
#if defined(__cplusplus)
#include <meshoptimizer.h>
#else
#error "TODO"
#endif

#include <core/file.h>

#include <graphics/scene_renderer_uploader.h>

/************************************************
 *
 * GLTF Utils Functinos Declaration
 * 
 ***********************************************/
static bool
create_mesh_material_
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_material                                     *material,
  _In_ crude_gfx_mesh                                     *mesh_draw,
  _In_ size_t                                             scene_renderer_images_offset,
  _In_ size_t                                             scene_renderer_samplers_offset
);

static cgltf_data*
parse_gltf_
(
  _In_ crude_stack_allocator                              *temporary_allocator,
  _In_ char const                                         *gltf_path
);

static void
load_images_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
);

static void
load_samplers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
);

static void
load_buffers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
);

static void
load_meshes_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory,
  _In_ uint32                                              scene_renderer_buffers_offset,
  _In_ uint32                                              scene_renderer_images_offset,
  _In_ uint32                                              scene_renderer_samplers_offset
);

static void
load_meshlets_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_stack_allocator                              *temporary_allocator
);

static void
load_meshlet_vertices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ crude_gfx_meshlet_vertex                          **meshletes_vertices
);

static void
load_meshlet_vertices_indices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ uint32                                            **submesh_vertices_indices
);

static void
build_meshlets_
(
  _In_ crude_gfx_meshlet_vertex                           *mesh_vertices,
  _In_ uint32                                              mesh_vertices_offset,
  _In_ uint32                                             *submesh_vertices_indices,
  _In_ uint32                                            **mesh_vertices_indices,
  _In_ uint8                                             **mesh_primitives_indices,
  _In_ crude_gfx_meshlet                                 **mesh_meshlets,
  _In_ crude_stack_allocator                              *temporary_allocator
);

void
crude_scene_renderer_upload_gltf
(
  _In_ crude_gfx_scene_renderer                          *scene_renderer,
  _In_ char const                                        *gltf_path,
  _In_ crude_entity                                       parent_node,
  _In_ crude_stack_allocator                             *temporary_allocator
)
{
  cgltf_data                                              *gltf;
  crude_string_buffer                                      temporary_string_buffer;
  char                                                     gltf_directory[ 512 ];
  uint32                                                   temporary_allocator_mark;
  size_t                                                   scene_renderer_images_offset;
  size_t                                                   scene_renderer_samplers_offset;
  size_t                                                   scene_renderer_buffers_offset;

  temporary_allocator_mark = crude_stack_allocator_get_marker( temporary_allocator );
  
  /* Parse gltf */
  gltf = parse_gltf_( temporary_allocator, gltf_path );
  if ( !gltf )
  {
    crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_mark );
    return;
  }
  
  /* Initialize tmp */
  crude_string_buffer_initialize( &temporary_string_buffer, 1024, crude_stack_allocator_pack( temporary_allocator ) );
  
  /* Get gltf directory */
  crude_memory_copy( gltf_directory, gltf_path, sizeof( gltf_directory ) );
  crude_file_directory_from_path( gltf_directory );
  
  /* Get scene renderer offsets */
  scene_renderer_images_offset = CRUDE_ARRAY_LENGTH( scene_renderer->images );
  scene_renderer_samplers_offset = CRUDE_ARRAY_LENGTH( scene_renderer->samplers );
  scene_renderer_buffers_offset = CRUDE_ARRAY_LENGTH( scene_renderer->buffers );

  /* Load gltf resources */
  load_images_( scene_renderer, gltf, &temporary_string_buffer, gltf_directory );
  load_samplers_( scene_renderer, gltf, &temporary_string_buffer, gltf_directory );
  load_buffers_( scene_renderer, gltf, &temporary_string_buffer, gltf_directory );
  load_meshes_( scene_renderer, gltf, &temporary_string_buffer, gltf_directory, scene_renderer_buffers_offset, scene_renderer_images_offset, scene_renderer_samplers_offset );
  load_meshlets_( scene_renderer, gltf, temporary_allocator );

  cgltf_free( gltf );
  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_mark );
}


/************************************************
 *
 * GLTF Utils Functinos Implementation
 * 
 ***********************************************/
bool
create_mesh_material_
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_material                                     *material,
  _In_ crude_gfx_mesh                                     *mesh_draw,
  _In_ size_t                                             scene_renderer_images_offset,
  _In_ size_t                                             scene_renderer_samplers_offset
)
{
  bool transparent = false;
  
  mesh_draw->flags = 0;

  mesh_draw->base_color_factor = CRUDE_COMPOUNT( crude_float4, {
    material->pbr_metallic_roughness.base_color_factor[ 0 ],
    material->pbr_metallic_roughness.base_color_factor[ 1 ],
    material->pbr_metallic_roughness.base_color_factor[ 2 ],
    material->pbr_metallic_roughness.base_color_factor[ 3 ],
  } );
  
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
    crude_gfx_renderer_texture *albedo_texture_gpu = &scene_renderer->images[ scene_renderer_images_offset + cgltf_image_index( gltf, albedo_texture->image ) ];
    crude_gfx_renderer_sampler *albedo_sampler_gpu = &scene_renderer->samplers[ scene_renderer_samplers_offset + cgltf_sampler_index( gltf, albedo_texture->sampler ) ];
  
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
    crude_gfx_renderer_texture *roughness_texture_gpu = &scene_renderer->images[ scene_renderer_images_offset + cgltf_image_index( gltf, roughness_texture->image ) ];
    crude_gfx_renderer_sampler *roughness_sampler_gpu = &scene_renderer->samplers[ scene_renderer_samplers_offset + cgltf_sampler_index( gltf, roughness_texture->sampler ) ];
    
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
    crude_gfx_renderer_texture *occlusion_texture_gpu = &scene_renderer->images[ scene_renderer_images_offset + cgltf_image_index( gltf, occlusion_texture->image ) ];
    crude_gfx_renderer_sampler *occlusion_sampler_gpu = &scene_renderer->samplers[ scene_renderer_samplers_offset + cgltf_sampler_index( gltf, occlusion_texture->sampler ) ];
    
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
    crude_gfx_renderer_texture *normal_texture_gpu = &scene_renderer->images[ scene_renderer_images_offset + cgltf_image_index( gltf, normal_texture->image ) ];
    crude_gfx_renderer_sampler *normal_sampler_gpu = &scene_renderer->samplers[ scene_renderer_samplers_offset + cgltf_sampler_index( gltf, normal_texture->sampler ) ];
    
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

cgltf_data*
parse_gltf_
(
  _In_ crude_stack_allocator                              *temporary_allocator,
  _In_ char const                                         *gltf_path
)
{
  cgltf_data                                              *gltf;
  cgltf_result                                             result;
  cgltf_options                                            gltf_options;
  crude_allocator_container                                temporary_allocator_container;

  temporary_allocator_container = crude_stack_allocator_pack( temporary_allocator );

  gltf_options = CRUDE_COMPOUNT( cgltf_options, { 
    .memory = {
      .alloc_func = temporary_allocator_container.allocate,
      .free_func  = temporary_allocator_container.deallocate,
      .user_data = temporary_allocator_container.ctx
    },
  } );

  gltf = NULL;
  result = cgltf_parse_file( &gltf_options, gltf_path, &gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_load_buffers( &gltf_options, gltf, gltf_path );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to load buffers from gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_validate( gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to validate gltf file: %s", gltf_path );
    return NULL;
  }

  return gltf;
}

static void
load_images_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
)
{
  for ( uint32 image_index = 0; image_index < gltf->images_count; ++image_index )
  {
    crude_gfx_renderer_texture                            *texture_resource;
    crude_gfx_texture_creation                             texture_creation;
    cgltf_image const                                     *image;
    char                                                  *image_full_filename;
    int                                                    comp, width, height;
    
    image = &gltf->images[ image_index ];
    image_full_filename = crude_string_buffer_append_use_f( temporary_string_buffer, "%s%s", gltf_directory, image->uri );
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

    texture_resource = crude_gfx_renderer_create_texture( scene_renderer->renderer, &texture_creation );
    CRUDE_ARRAY_PUSH( scene_renderer->images, *texture_resource );
    crude_gfx_asynchronous_loader_request_texture_data( scene_renderer->async_loader, image_full_filename, texture_resource->handle );
  }
}

void
load_samplers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
)
{
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
    
    sampler_resource = crude_gfx_renderer_create_sampler( scene_renderer->renderer, &creation );
    CRUDE_ARRAY_PUSH( scene_renderer->samplers, *sampler_resource );
  }
}

void
load_buffers_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory
)
{
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
      buffer_name = crude_string_buffer_append_use_f( temporary_string_buffer, "scene_renderer_buffer%i", buffer_view_index );
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
    cpu_buffer = crude_gfx_create_buffer( scene_renderer->renderer->gpu, &cpu_buffer_creation );

    gpu_buffer_creation = crude_gfx_buffer_creation_empty();
    gpu_buffer_creation.usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE;
    gpu_buffer_creation.size = buffer_view->size;
    gpu_buffer_creation.type_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    gpu_buffer_creation.name = buffer_name;
    gpu_buffer_creation.device_only = true;
    gpu_buffer_resource = crude_gfx_renderer_create_buffer( scene_renderer->renderer, &gpu_buffer_creation );
    CRUDE_ARRAY_PUSH( scene_renderer->buffers, *gpu_buffer_resource );

    crude_gfx_asynchronous_loader_request_buffer_copy( scene_renderer->async_loader, cpu_buffer, gpu_buffer_resource->handle );

    crude_string_buffer_clear( temporary_string_buffer );
  }
}

void
load_meshes_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_string_buffer                                *temporary_string_buffer,
  _In_ char const                                         *gltf_directory,
  _In_ uint32                                              scene_renderer_buffers_offset,
  _In_ uint32                                              scene_renderer_images_offset,
  _In_ uint32                                              scene_renderer_samplers_offset
)
{
  crude_float3 const node_scale = { 1.0f, 1.0f, 1.0f };
  crude_float3 const node_translation = { 1.0f, 1.0f, 1.0f };
  crude_float4 const node_rotation = { 1.0f, 1.0f, 1.0f };
  for ( uint32 mesh_index = 0; mesh_index < gltf->meshes_count; ++mesh_index )
  {
    cgltf_mesh *mesh = &gltf->meshes[ mesh_index ];
    for ( uint32 primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index )
    {
      crude_gfx_mesh                                       mesh_draw;
      cgltf_primitive                                     *mesh_primitive;
      cgltf_accessor                                      *indices_accessor;
      cgltf_buffer_view                                   *indices_buffer_view;
      crude_gfx_renderer_buffer                           *indices_buffer_gpu;
      bool                                                 material_transparent;
      
      mesh_primitive = &mesh->primitives[ primitive_index ];

      mesh_draw = CRUDE_COMPOUNT_EMPTY( crude_gfx_mesh );
      for ( uint32 i = 0; i < mesh_primitive->attributes_count; ++i )
      {
        crude_gfx_renderer_buffer *buffer_gpu = &scene_renderer->buffers[ scene_renderer_buffers_offset + cgltf_buffer_view_index( gltf, mesh_primitive->attributes[ i ].data->buffer_view ) ];
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
      
      indices_buffer_gpu = &scene_renderer->buffers[ scene_renderer_buffers_offset + cgltf_buffer_view_index( gltf, indices_accessor->buffer_view ) ];

      material_transparent = create_mesh_material_( gltf, scene_renderer->renderer, scene_renderer, mesh_primitive->material, &mesh_draw, scene_renderer_images_offset, scene_renderer_samplers_offset );
      
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
      CRUDE_ARRAY_PUSH( scene_renderer->meshes, mesh_draw );
    }
  }
}

void
load_meshlets_
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_data                                         *gltf,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  uint32                                              *submesh_vertices_indices;
  uint64                                               temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( submesh_vertices_indices, 0u, crude_stack_allocator_pack( temporary_allocator ) );

  for ( uint32 mesh_index = 0; mesh_index < gltf->meshes_count; ++mesh_index )
  {
    cgltf_mesh *mesh = &gltf->meshes[ mesh_index ];
    for ( uint32 primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index )
    {
      cgltf_primitive                                     *mesh_primitive;
      uint32                                               meshlets_vertices_offset;
      
      mesh_primitive = &mesh->primitives[ primitive_index ];
      meshlets_vertices_offset = CRUDE_ARRAY_LENGTH( scene_renderer->meshlets_vertices );

      load_meshlet_vertices_( mesh_primitive, &scene_renderer->meshlets_vertices );
      load_meshlet_vertices_indices_( mesh_primitive, &submesh_vertices_indices );
      build_meshlets_( scene_renderer->meshlets_vertices, meshlets_vertices_offset, submesh_vertices_indices, &scene_renderer->meshlets_vertices_indices, &scene_renderer->meshlets_triangles_indices, &scene_renderer->meshlets, temporary_allocator );
    }
  }

  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
}

static void
load_meshlet_vertices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ crude_gfx_meshlet_vertex                          **meshletes_vertices
)
{
  crude_float3                                            *primitive_positions;
  crude_float3                                            *primitive_normals;
  crude_float2                                            *primitive_texcoords;
  uint32                                                   meshlet_vertices_count;

  meshlet_vertices_count = primitive->attributes[ 0 ].data->count;
  
  for ( uint32 i = 0; i < primitive->attributes_count; ++i )
  {
    cgltf_attribute *attribute = &primitive->attributes[ i ];
    CRUDE_ASSERT( meshlet_vertices_count == attribute->data->count );

    uint8 *attribute_data = CRUDE_STATIC_CAST( uint8*, attribute->data->buffer_view->buffer->data ) + attribute->data->buffer_view->offset;
    switch ( attribute->type )
    {
    case cgltf_attribute_type_position:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec3 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( crude_float3 ) );
      primitive_positions = CRUDE_CAST( crude_float3*, attribute_data );
      break;
    }
    case cgltf_attribute_type_normal:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec3 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( crude_float3 ) );
      primitive_normals = CRUDE_CAST( crude_float3*, attribute_data );
      break;
    }
    case cgltf_attribute_type_texcoord:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec2 );
      CRUDE_ASSERT( attribute->data->stride == sizeof( crude_float2 ) );
      primitive_texcoords = CRUDE_CAST( crude_float2*, attribute_data );
      break;
    }
    }
  }

  CRUDE_ARRAY_SET_CAPACITY( *meshletes_vertices, CRUDE_ARRAY_LENGTH( *meshletes_vertices ) + meshlet_vertices_count );
  for ( uint32 i = 0; i < meshlet_vertices_count; ++i )
  {
    CRUDE_ASSERT( primitive_positions );
    CRUDE_ASSERT( primitive_normals );
    CRUDE_ASSERT( primitive_texcoords );
    
    crude_gfx_meshlet_vertex new_meshlet_vertex;
    new_meshlet_vertex.position.x = primitive_positions[ i ].x;
    new_meshlet_vertex.position.y = primitive_positions[ i ].y;
    new_meshlet_vertex.position.z = primitive_positions[ i ].z;
    new_meshlet_vertex.normal[ 0 ] = ( primitive_normals[ i ].x + 1.0f ) * 127.0f;
    new_meshlet_vertex.normal[ 1 ] = ( primitive_normals[ i ].y + 1.0f ) * 127.0f;
    new_meshlet_vertex.normal[ 2 ] = ( primitive_normals[ i ].z + 1.0f ) * 127.0f;
    new_meshlet_vertex.texcoords[ 0 ] = meshopt_quantizeHalf( primitive_texcoords[ i ].x );
    new_meshlet_vertex.texcoords[ 1 ] = meshopt_quantizeHalf( primitive_texcoords[ i ].y );
    CRUDE_ARRAY_PUSH( *meshletes_vertices, new_meshlet_vertex );
  }
}

static void
load_meshlet_vertices_indices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ uint32                                            **submesh_vertices_indices
)
{
  uint16                                                  *primitive_indices;
  uint32                                                   meshlet_vertices_indices_count;
  uint8                                                   *buffer_data;

  meshlet_vertices_indices_count = primitive->indices->count;
  CRUDE_ARRAY_SET_LENGTH( *submesh_vertices_indices, meshlet_vertices_indices_count );
  
  CRUDE_ASSERT( primitive->indices->type == cgltf_type_scalar );
  CRUDE_ASSERT( primitive->indices->component_type == cgltf_component_type_r_16u );

  buffer_data = CRUDE_CAST( uint8*, primitive->indices->buffer_view->buffer->data ) + primitive->indices->buffer_view->offset;
  primitive_indices = CRUDE_CAST( uint16*, buffer_data );

  for ( uint32 i = 0; i < meshlet_vertices_indices_count; ++i )
  {
    ( *submesh_vertices_indices )[ i ] = primitive_indices[ i ];
  }
}

void
build_meshlets_
(
  _In_ crude_gfx_meshlet_vertex                           *mesh_vertices,
  _In_ uint32                                              mesh_vertices_offset,
  _In_ uint32                                             *submesh_vertices_indices,
  _In_ uint32                                            **mesh_vertices_indices,
  _In_ uint8                                             **mesh_primitives_indices,
  _In_ crude_gfx_meshlet                                 **mesh_meshlets,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  // !TODO custom allocator meshopt_setAllocator
#if defined(__cplusplus)
  size_t                                                   max_vertices = 64u;
  size_t                                                   max_primitives = 124u;
  float32                                                  cone_weight = 0.0f;
  
  meshopt_Meshlet const                                   *zeux_last_meshlet;
  meshopt_Meshlet                                         *zeux_meshlets;
  uint32                                                   mesh_vertices_indices_offset, mesh_primitives_indices_offset, mesh_meshletes_offset;
  size_t                                                   max_meshlets, meshletes_count;

  mesh_vertices_indices_offset = CRUDE_ARRAY_LENGTH( *mesh_vertices_indices );
  mesh_primitives_indices_offset = CRUDE_ARRAY_LENGTH( *mesh_primitives_indices );
  mesh_meshletes_offset = CRUDE_ARRAY_LENGTH( *mesh_meshlets );
  max_meshlets = meshopt_buildMeshletsBound( CRUDE_ARRAY_LENGTH( submesh_vertices_indices ), max_vertices, max_primitives );

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( zeux_meshlets, max_meshlets, crude_stack_allocator_pack( temporary_allocator ) );

  CRUDE_ARRAY_SET_LENGTH( *mesh_vertices_indices, mesh_vertices_indices_offset + max_meshlets * max_vertices );
  CRUDE_ARRAY_SET_LENGTH( *mesh_primitives_indices, mesh_primitives_indices_offset + max_meshlets * max_primitives );

  meshletes_count = meshopt_buildMeshlets( zeux_meshlets, 
    &( *mesh_vertices_indices )[ mesh_vertices_indices_offset ], &( *mesh_primitives_indices )[ mesh_primitives_indices_offset ],
    submesh_vertices_indices, CRUDE_ARRAY_LENGTH( submesh_vertices_indices ), 
    &mesh_vertices[ mesh_vertices_offset ].position.x, CRUDE_ARRAY_LENGTH( mesh_vertices ) - mesh_vertices_offset, sizeof( crude_gfx_meshlet_vertex ), 
    max_vertices, max_primitives, cone_weight
  );

  zeux_last_meshlet = &zeux_meshlets[ meshletes_count - 1];
  CRUDE_ARRAY_SET_LENGTH( *mesh_vertices_indices, mesh_vertices_indices_offset + zeux_last_meshlet->vertex_offset + zeux_last_meshlet->vertex_count );
  CRUDE_ARRAY_SET_LENGTH( *mesh_primitives_indices, mesh_primitives_indices_offset + zeux_last_meshlet->triangle_offset + ( ( zeux_last_meshlet->triangle_count * 3 + 3 ) & ~3 ) );
  CRUDE_ARRAY_SET_LENGTH( zeux_meshlets, meshletes_count );

  CRUDE_ARRAY_SET_CAPACITY( *mesh_meshlets, mesh_meshletes_offset + meshletes_count );

  for ( uint32 meshlet_index = 0; meshlet_index < meshletes_count; ++meshlet_index )
  {
    meshopt_Meshlet const                                 *current_zeux_meshlet;
    crude_gfx_meshlet                                      new_meshlet;

    current_zeux_meshlet = &zeux_meshlets[ meshlet_index ];
    meshopt_optimizeMeshlet(
      &( *mesh_vertices_indices )[ mesh_vertices_indices_offset + current_zeux_meshlet->vertex_offset ],
      &( *mesh_primitives_indices )[ mesh_primitives_indices_offset + current_zeux_meshlet->triangle_offset ],
      current_zeux_meshlet->triangle_count,
      current_zeux_meshlet->vertex_count
    );

    new_meshlet = {
      .vertices_offset = CRUDE_STATIC_CAST( uint32, mesh_vertices_indices_offset + current_zeux_meshlet->vertex_offset ),
      .triangles_offset = CRUDE_STATIC_CAST( uint32, mesh_primitives_indices_offset + current_zeux_meshlet->triangle_offset ),
      .vertices_count = CRUDE_STATIC_CAST( uint8, current_zeux_meshlet->vertex_count ),
      .triangles_count = CRUDE_STATIC_CAST( uint8, current_zeux_meshlet->triangle_count ),
    };
    CRUDE_ARRAY_PUSH( *mesh_meshlets, new_meshlet );
  }
#else
#error "TODO"
#endif
}