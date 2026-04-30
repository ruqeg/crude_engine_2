#include <thirdparty/cgltf/cgltf.h>
#include <thirdparty/stb/stb_image.h>
#include <meshoptimizer.h>

#include <engine/core/file.h>
#include <engine/core/hashmapstr.h>
#include <engine/core/memory.h>
#include <engine/core/profiler.h>

#include <engine/graphics/model_renderer_resources_manager.h>

/**
 * Scene Renderer Register Nodes & Gltf & Lights
 */
static uint32
crude_gfX_get_gpu_mesh_global_index_
(
  _In_ uint32                                              old_total_meshes_count,
  _In_ uint32                                              cgltf_mesh_primitive_index
)
{
  return old_total_meshes_count + cgltf_mesh_primitive_index;
}

static crude_gfx_model_renderer_resources
crude_gfx_model_renderer_resources_manager_load_gltf_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ char const                                         *gltf_relative_filepath
);

static void
crude_gfx_model_renderer_resources_manager_gltf_write_mesh_material_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Out_ crude_gfx_mesh_cpu                                *mesh_draw,
  _In_ cgltf_data                                         *gltf,
  _In_ cgltf_material                                     *material,
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
  _In_ char const                                         *gltf_directory
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_samplers_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ char const                                         *gltf_directory
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_textures_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf
);

static uint64
crude_gfx_model_renderer_resources_manager_gltf_calculate_meshes_count_
(
  _In_ cgltf_data                                         *gltf
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_geometry_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Out_ crude_gfx_model_renderer_resources                *model_renderer_resouces,
  _Out_ uint32                                            *gltf_mesh_index_to_mesh_primitive_index,
  _In_ cgltf_data                                         *gltf,
  _In_ char const                                         *gltf_absolute_directory,
  _In_ uint32                                              images_offset,
  _In_ uint32                                              samplers_offset
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_nodes_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources,
  _In_ cgltf_data                                         *gltf,
  _In_ uint32                                             *gltf_mesh_index_to_mesh_primitive_index
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_vertices_
(
  _In_ cgltf_primitive                                    *primitive,
  _Out_ crude_gfx_vertex                                  *vertices,
  _Out_ crude_gfx_vertex_position                         *vertices_positions,
  _Out_ crude_gfx_vertex_joint                            *vertices_joints
);

static void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_indices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ uint32                                             *indices
);

static void
crude_gfx_model_renderer_resources_manager_load_skins_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources,
  _In_ cgltf_data                                         *gltf
);

static void
crude_gfx_model_renderer_resources_manager_load_animations_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources,
  _In_ cgltf_data                                         *gltf
);

#if CRUDE_GFX_RAY_TRACING_ENABLED
static void
crude_gfx_model_renderer_resources_manager_create_bottom_level_acceleration_structure_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources
);
#endif

void
crude_gfx_model_renderer_resources_manager_intialize
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager,
  _In_ crude_gfx_model_renderer_resources_manager_creation const *creation
)
{
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Initialize model resources manager." );

  manager->async_loader = creation->async_loader;
  manager->allocator = creation->allocator;
  manager->cgltf_temporary_allocator = creation->cgltf_temporary_allocator;
  manager->temporary_allocator = creation->temporary_allocator;
  manager->gpu = creation->async_loader->gpu;
  manager->resources_absolute_directory = creation->resources_absolute_directory;
#if CRUDE_DEVELOP
  manager->test_allocator = creation->test_allocator;
#endif
  manager->texture_manager = creation->texture_manager;

  manager->total_meshes_count = 0;

  manager->total_meshlets_count = 0;
  manager->total_meshlets_vertices_count = 0;
  manager->total_meshlets_vertices_indices_count = 0;
  manager->total_meshlets_triangles_indices_count = 0;

  manager->meshlets_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_vertices_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_vertices_joints_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_vertices_positions_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_vertices_indices_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_triangles_indices_hga = crude_gfx_memory_allocation_empty( );

  manager->meshes_draws_hga = crude_gfx_memory_allocation_empty( );

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( manager->samplers, 0u, crude_heap_allocator_pack( creation->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( manager->images, 0u, crude_heap_allocator_pack( creation->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( manager->indices_buffers, 0u, crude_heap_allocator_pack( creation->allocator ) );

  CRUDE_HASHMAPSTR_INITIALIZE( manager->model_name_to_model_renderer_resource, crude_heap_allocator_pack( creation->allocator ) );

  crude_resource_pool_initialize( &manager->model_renderer_resources_pool, crude_heap_allocator_pack( creation->allocator ), 1024, sizeof( crude_gfx_model_renderer_resources ) );

  crude_linear_allocator_initialize( &manager->linear_allocator, CRUDE_RKILO( 32 ) + 2, "crude_gfx_model_renderer_resources_manager::linear_allocator" );
  crude_string_buffer_initialize( &manager->gltf_absolute_filepath_string_buffer, CRUDE_RKILO( 16 ), crude_linear_allocator_pack( &manager->linear_allocator ) );
  crude_string_buffer_initialize( &manager->image_absolute_filepath_string_buffer, CRUDE_RKILO( 16 ), crude_linear_allocator_pack( &manager->linear_allocator ) );
  
  {
    VkTransformMatrixKHR vk_transform_matrix = CRUDE_COMPOUNT( VkTransformMatrixKHR, {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f
    } );
    manager->bottom_level_acceleration_structure_transform_hga = crude_gfx_memory_allocate_with_name( manager->gpu, sizeof( vk_transform_matrix ), CRUDE_GFX_MEMORY_TYPE_CPU_GPU, "bottom_level_acceleration_structure_transform_hga", VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );
    crude_memory_copy( manager->bottom_level_acceleration_structure_transform_hga.cpu_address, &vk_transform_matrix, sizeof( vk_transform_matrix ) );
  }
}

void
crude_gfx_model_renderer_resources_manager_deintialize
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager
)
{
  crude_gfx_memory_deallocate( manager->gpu, manager->bottom_level_acceleration_structure_transform_hga );

  for ( uint32 i = 0; i < CRUDE_HASHMAPSTR_CAPACITY( manager->model_name_to_model_renderer_resource ); ++i )
  {
    if ( crude_hashmapstr_backet_key_hash_valid( manager->model_name_to_model_renderer_resource[ i ].key.key_hash ) )
    {
      crude_gfx_model_renderer_resources                  *resource;
      crude_gfx_model_renderer_resources_handle            resource_handle;
      
      resource_handle = manager->model_name_to_model_renderer_resource[ i ].value;
      resource = CRUDE_CAST( crude_gfx_model_renderer_resources*, crude_resource_pool_access_resource( &manager->model_renderer_resources_pool, resource_handle.index ) );
      
      crude_gfx_model_renderer_resources_deinitialize( manager->gpu, resource );
      crude_resource_pool_release_resource( &manager->model_renderer_resources_pool, resource_handle.index );
    }
  }
  CRUDE_HASHMAPSTR_DEINITIALIZE( manager->model_name_to_model_renderer_resource );
  crude_resource_pool_deinitialize( &manager->model_renderer_resources_pool );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->samplers ); ++i )
  {
    crude_gfx_destroy_sampler( manager->gpu, manager->samplers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( manager->samplers );

  /* Texture manager handle cleaning */
  CRUDE_ARRAY_DEINITIALIZE( manager->images );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->indices_buffers ); ++i )
  {
    crude_gfx_memory_deallocate( manager->gpu, manager->indices_buffers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( manager->indices_buffers );
  
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_vertices_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_vertices_joints_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_vertices_positions_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_vertices_indices_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_triangles_indices_hga );

  crude_gfx_memory_deallocate( manager->gpu, manager->meshes_draws_hga );

  crude_linear_allocator_deinitialize( &manager->linear_allocator );
  crude_string_buffer_deinitialize( &manager->gltf_absolute_filepath_string_buffer );
  crude_string_buffer_deinitialize( &manager->image_absolute_filepath_string_buffer );
}

void
crude_gfx_model_renderer_resources_manager_clear
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager
)
{
  for ( uint32 i = 0; i < CRUDE_HASHMAPSTR_CAPACITY( manager->model_name_to_model_renderer_resource ); ++i )
  {
    if ( crude_hashmapstr_backet_key_hash_valid( manager->model_name_to_model_renderer_resource[ i ].key.key_hash ) )
    {
      crude_gfx_model_renderer_resources                  *resource;
      crude_gfx_model_renderer_resources_handle            resource_handle;
      
      resource_handle = manager->model_name_to_model_renderer_resource[ i ].value;
      resource = CRUDE_CAST( crude_gfx_model_renderer_resources*, crude_resource_pool_access_resource( &manager->model_renderer_resources_pool, resource_handle.index ) );
      
      crude_gfx_model_renderer_resources_deinitialize( manager->gpu, resource );
      crude_resource_pool_release_resource( &manager->model_renderer_resources_pool, resource_handle.index );
    }
    manager->model_name_to_model_renderer_resource[ i ].key.key_hash = CRUDE_HASHMAPSTR_BACKET_STATE_EMPTY;
  }

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->samplers ); ++i )
  {
    crude_gfx_destroy_sampler( manager->gpu, manager->samplers[ i ] );
  }
  CRUDE_ARRAY_SET_LENGTH( manager->samplers, 0 );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( manager->indices_buffers ); ++i )
  {
    crude_gfx_memory_deallocate( manager->gpu, manager->indices_buffers[ i ] );
  }
  CRUDE_ARRAY_SET_LENGTH( manager->indices_buffers, 0 );

  /* Texture manager will handle cleaning */
  CRUDE_ARRAY_SET_LENGTH( manager->images, 0 );

  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_vertices_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_vertices_joints_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_vertices_positions_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_vertices_indices_hga );
  crude_gfx_memory_deallocate( manager->gpu, manager->meshlets_triangles_indices_hga );

  crude_gfx_memory_deallocate( manager->gpu, manager->meshes_draws_hga );

  manager->total_meshes_count = 0;

  manager->total_meshlets_count = 0;
  manager->total_meshlets_vertices_count = 0;
  manager->total_meshlets_vertices_indices_count = 0;
  manager->total_meshlets_triangles_indices_count = 0;

  manager->meshlets_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_vertices_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_vertices_joints_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_vertices_positions_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_vertices_indices_hga = crude_gfx_memory_allocation_empty( );
  manager->meshlets_triangles_indices_hga = crude_gfx_memory_allocation_empty( );

  manager->meshes_draws_hga = crude_gfx_memory_allocation_empty( );
}

crude_gfx_model_renderer_resources_handle
crude_gfx_model_renderer_resources_manager_get_gltf_model
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager,
  _In_ char const                                          *filepath
)
{
  crude_gfx_model_renderer_resources_handle                model_renderer_resouces_handle;
  crude_gfx_model_renderer_resources                      *model_renderer_resouces;
  int64                                                    model_renderer_resouces_index;

  model_renderer_resouces_index = CRUDE_HASHMAPSTR_GET_INDEX( manager->model_name_to_model_renderer_resource, filepath );
  if ( model_renderer_resouces_index != -1 )
  {
    return manager->model_name_to_model_renderer_resource[ model_renderer_resouces_index ].value;
  }
  
  model_renderer_resouces_handle = { crude_resource_pool_obtain_resource( &manager->model_renderer_resources_pool ) };
  model_renderer_resouces = CRUDE_CAST( crude_gfx_model_renderer_resources*, crude_resource_pool_access_resource( &manager->model_renderer_resources_pool, model_renderer_resouces_handle.index ) );
  
  crude_string_copy( model_renderer_resouces->relative_filepath, filepath, sizeof( model_renderer_resouces->relative_filepath ) );

  *model_renderer_resouces = crude_gfx_model_renderer_resources_manager_load_gltf_( manager, model_renderer_resouces->relative_filepath );
  CRUDE_HASHMAPSTR_SET( manager->model_name_to_model_renderer_resource, CRUDE_COMPOUNT( crude_string_link, { model_renderer_resouces->relative_filepath } ), model_renderer_resouces_handle );
  return model_renderer_resouces_handle;
}

void
crude_gfx_model_renderer_resources_manager_wait_till_uploaded
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_cmd_buffer                               *cmd
)
{
  CRUDE_PROFILER_ZONE_NAME( "crude_gfx_model_renderer_resources_manager_wait_till_uploaded" );
  while ( crude_gfx_asynchronous_loader_has_requests( manager->async_loader ) )
  {
    crude_gfx_add_texture_update_commands( manager->gpu, cmd );
  }
  CRUDE_PROFILER_ZONE_END;
}

crude_gfx_model_renderer_resources*
crude_gfx_model_renderer_resources_manager_access_model_renderer_resources
(
  _In_ crude_gfx_model_renderer_resources_manager          *manager,
  _In_ crude_gfx_model_renderer_resources_handle            handle
)
{
  if ( handle.index == -1 )
  {
    return NULL;
  }
  return CRUDE_CAST( crude_gfx_model_renderer_resources*, crude_resource_pool_access_resource( &manager->model_renderer_resources_pool, handle.index ) );
}

/**
 * Register Nodes
 */
crude_gfx_model_renderer_resources
crude_gfx_model_renderer_resources_manager_load_gltf_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ char const                                         *gltf_relative_filepath
)
{
  cgltf_data                                              *gltf;
  uint32                                                  *gltf_mesh_index_to_mesh_primitive_index;
  char                                                    *gltf_absolute_directory;
  char                                                    *gltf_absolute_filepath;
  crude_gfx_model_renderer_resources                       model_renderer_resouces;
  uint64                                                   temporary_allocator_marker, images_offset, samplers_offset;

  temporary_allocator_marker = crude_stack_allocator_get_marker( manager->temporary_allocator );

  model_renderer_resouces = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources );
  
  crude_string_copy( model_renderer_resouces.relative_filepath, gltf_relative_filepath, sizeof( model_renderer_resouces.relative_filepath ) );

  images_offset = CRUDE_ARRAY_LENGTH( manager->images );
  samplers_offset = CRUDE_ARRAY_LENGTH( manager->samplers );

  /* Parse gltf */
  gltf_absolute_filepath = crude_string_buffer_append_use_f( &manager->gltf_absolute_filepath_string_buffer, "%s%s", manager->resources_absolute_directory, gltf_relative_filepath );
  gltf = crude_gfx_model_renderer_resources_manager_gltf_parse_( manager->cgltf_temporary_allocator, gltf_absolute_filepath );
  if ( !gltf )
  {
    goto cleanup;
  }
  
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading \"%s\" gltf", gltf_absolute_filepath );

  gltf_absolute_directory = gltf_absolute_filepath;
  gltf_absolute_filepath = NULL;
  crude_file_directory_from_path( gltf_absolute_directory );
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( gltf_mesh_index_to_mesh_primitive_index, gltf->meshes_count, crude_stack_allocator_pack( manager->temporary_allocator ) );
  
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading images" );
  crude_gfx_model_renderer_resources_manager_gltf_load_images_( manager, gltf, gltf_absolute_directory );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading samplers" );
  crude_gfx_model_renderer_resources_manager_gltf_load_samplers_( manager, gltf, gltf_absolute_directory );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading textures" );
  crude_gfx_model_renderer_resources_manager_gltf_load_textures_( manager, gltf ); /* Should be executed after images/samplers loaded*/
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading geometry" );
  crude_gfx_model_renderer_resources_manager_gltf_load_geometry_( manager, &model_renderer_resouces, gltf_mesh_index_to_mesh_primitive_index, gltf, gltf_absolute_directory, images_offset, samplers_offset );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading skins" );
  crude_gfx_model_renderer_resources_manager_load_skins_( manager, &model_renderer_resouces, gltf );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading nodes" );
  crude_gfx_model_renderer_resources_manager_gltf_load_nodes_( manager, &model_renderer_resouces, gltf, gltf_mesh_index_to_mesh_primitive_index );
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading animations" );
  crude_gfx_model_renderer_resources_manager_load_animations_( manager, &model_renderer_resouces, gltf );

#if CRUDE_GFX_RAY_TRACING_ENABLED
  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Create bottom level acceleration structure" );
  // !BACK crude_gfx_model_renderer_resources_manager_create_bottom_level_acceleration_structure_( manager, &model_renderer_resouces );
#endif

  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading finished" );

cleanup:
  if ( gltf )
  {
    cgltf_free( gltf );
  }
  crude_stack_allocator_free_marker( manager->temporary_allocator, temporary_allocator_marker );
  crude_string_buffer_clear( &manager->gltf_absolute_filepath_string_buffer );

  return model_renderer_resouces;
}


/************************************************
 *
 * GLTF Utils Functinos Implementation
 * 
 ***********************************************/
void
crude_gfx_model_renderer_resources_manager_gltf_write_mesh_material_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Out_ crude_gfx_mesh_cpu                                *mesh_draw,
  _In_ cgltf_data                                         *gltf,
  _In_ cgltf_material                                     *material,
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
  mesh_draw->albedo_texture_handle.index = CRUDE_SHADER_TEXTURE_UNDEFINED;
  mesh_draw->metallic_roughness_texture_handle.index = CRUDE_SHADER_TEXTURE_UNDEFINED;
  mesh_draw->occlusion_texture_handle.index = CRUDE_SHADER_TEXTURE_UNDEFINED;
  mesh_draw->normal_texture_handle.index = CRUDE_SHADER_TEXTURE_UNDEFINED;

  if ( material == NULL )
  {
    return;
  }

  mesh_draw->emmision.x = material->emissive_factor[ 0 ];
  mesh_draw->emmision.y = material->emissive_factor[ 1 ];
  mesh_draw->emmision.z = material->emissive_factor[ 2 ];
  mesh_draw->emmision.w = material->emissive_strength.emissive_strength;

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
    mesh_draw->flags |= CRUDE_MESH_DRAW_FLAGS_ALPHA_MASK;
  }
  else if ( material->alpha_mode == cgltf_alpha_mode_blend )
  {
    mesh_draw->flags |= CRUDE_MESH_DRAW_FLAGS_TRANSLUCENT_MASK;
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
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Failed to parse gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_load_buffers( &gltf_options, gltf, gltf_path );
  if ( result != cgltf_result_success )
  {
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Failed to load buffers from gltf file: %s", gltf_path );
    return NULL;
  }

  result = cgltf_validate( gltf );
  if ( result != cgltf_result_success )
  {
    CRUDE_ASSERTM( CRUDE_CHANNEL_GRAPHICS, false, "Failed to validate gltf file: %s", gltf_path );
    return NULL;
  }

  return gltf;
}

static void
crude_gfx_model_renderer_resources_manager_gltf_load_images_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
  _In_ char const                                         *gltf_directory
)
{
  for ( uint32 image_index = 0; image_index < gltf->images_count; ++image_index )
  {
    crude_gfx_texture_handle                               texture_handle;
    crude_gfx_texture_creation                             texture_creation;
    cgltf_image const                                     *image;
    char                                                  *image_absolute_filename;
    int                                                    comp, width, height;
    
    image = &gltf->images[ image_index ];
    image_absolute_filename = crude_string_buffer_append_use_f( &manager->image_absolute_filepath_string_buffer, "%s%s", gltf_directory, image->uri );

    texture_handle = crude_gfx_texture_manager_get_texture( manager->texture_manager, image->uri, image_absolute_filename );
    CRUDE_ARRAY_PUSH( manager->images, texture_handle );
    crude_string_buffer_clear( &manager->image_absolute_filepath_string_buffer );
  }
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_samplers_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ cgltf_data                                         *gltf,
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
  _In_ crude_gfx_model_renderer_resources_manager          *manager,
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

uint64
crude_gfx_model_renderer_resources_manager_gltf_calculate_meshes_count_
(
  _In_ cgltf_data                                         *gltf
)
{
  uint64 meshes_count = 0;
  for ( uint32 mesh_index = 0; mesh_index < gltf->meshes_count; ++mesh_index )
  { 
    cgltf_mesh *mesh = &gltf->meshes[ mesh_index ];
    for ( uint32 primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index )
    {
      ++meshes_count;
    }
  }
  return meshes_count;
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_geometry_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _Out_ crude_gfx_model_renderer_resources                *model_renderer_resouces,
  _Out_ uint32                                            *gltf_mesh_index_to_mesh_primitive_index,
  _In_ cgltf_data                                         *gltf,
  _In_ char const                                         *gltf_absolute_directory,
  _In_ uint32                                              images_offset,
  _In_ uint32                                              samplers_offset
)
{
  crude_gfx_mesh_draw                                     *meshes_draws;
  crude_gfx_cmd_buffer                                    *immediate_transfer_cmd_buffer;
  uint64                                                   mesh_index;

  CRUDE_LOG_INFO( CRUDE_CHANNEL_GRAPHICS, "Loading geometry" );
  
  immediate_transfer_cmd_buffer = crude_gfx_access_cmd_buffer( manager->gpu, manager->gpu->immediate_transfer_cmd_buffer );
    
  crude_gfx_cmd_begin_primary( immediate_transfer_cmd_buffer );

  mesh_index = 0;
  for ( uint32 gltf_mesh_index = 0; gltf_mesh_index < gltf->meshes_count; ++gltf_mesh_index )
  {
    cgltf_mesh                                            *gltf_mesh;
    
    gltf_mesh = &gltf->meshes[ gltf_mesh_index ];

    gltf_mesh_index_to_mesh_primitive_index[ mesh_index ] = mesh_index;

    for ( uint32 gltf_primitive_index = 0; gltf_primitive_index < gltf_mesh->primitives_count; ++gltf_primitive_index )
    {
      ++mesh_index;
    }
  }

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( model_renderer_resouces->meshes, mesh_index, crude_heap_allocator_pack( manager->allocator ) );
  
  mesh_index = 0;
  for ( uint32 gltf_mesh_index = 0; gltf_mesh_index < gltf->meshes_count; ++gltf_mesh_index )
  {
    cgltf_mesh                                            *gltf_mesh;
    
    gltf_mesh = &gltf->meshes[ gltf_mesh_index ];

    for ( uint32 gltf_primitive_index = 0; gltf_primitive_index < gltf_mesh->primitives_count; ++gltf_primitive_index )
    {
      crude_gfx_mesh_cpu                                  *mesh_cpu;
      cgltf_primitive                                     *gltf_mesh_primitive;
      cgltf_accessor                                      *gltf_indices_accessor;
      XMVECTOR                                             bounding_center;
      float32                                              bounding_radius;
      uint32                                               flags;
      
      gltf_mesh_primitive = &gltf_mesh->primitives[ gltf_primitive_index ];
      mesh_cpu = &model_renderer_resouces->meshes[ mesh_index ];

      flags = 0u;

      for ( uint32 i = 0; i < gltf_mesh_primitive->attributes_count; ++i )
      {
        cgltf_attribute const                             *gltf_attribute;

        gltf_attribute = &gltf_mesh_primitive->attributes[ i ];

        switch ( gltf_mesh_primitive->attributes[ i ].type )
        {
        case cgltf_attribute_type_position:
        {
          XMVECTOR                                         position_max;
          XMVECTOR                                         position_min;
          
          CRUDE_ASSERT( sizeof( cgltf_float[4] ) == sizeof( XMFLOAT4 ) );
          CRUDE_ASSERT( gltf_mesh_primitive->attributes[ i ].data->has_max && gltf_attribute->data->has_min );
          
          position_max = XMLoadFloat4( CRUDE_REINTERPRET_CAST( XMFLOAT4 const*, gltf_attribute->data->max ) );
          position_min = XMLoadFloat4( CRUDE_REINTERPRET_CAST( XMFLOAT4 const*, gltf_attribute->data->min ) );

          bounding_center = XMVectorAdd( position_max, position_min );
          bounding_center = XMVectorScale( bounding_center, 0.5f );
          bounding_radius = XMVectorGetX( XMVectorMax( XMVector3Length( position_max - bounding_center ), XMVector3Length( position_min - bounding_center ) ) );
          break;
        }
        case cgltf_attribute_type_tangent:
        {
          flags |= CRUDE_MESH_DRAW_FLAGS_HAS_TANGENTS;
          break;
        }
        case cgltf_attribute_type_normal:
        {
          flags |= CRUDE_MESH_DRAW_FLAGS_HAS_NORMAL;
          break;
        }
        case cgltf_attribute_type_texcoord:
        {
          CRUDE_ASSERT( gltf_attribute->data->component_type == cgltf_component_type_r_32f );
          CRUDE_ASSERT( gltf_attribute->data->type == cgltf_type_vec2 );
          break;
        }
        }
      }
      
      gltf_indices_accessor = gltf_mesh_primitive->indices;
      
      if ( gltf_indices_accessor->component_type == cgltf_component_type_r_16u )
      {
        flags |= CRUDE_MESH_DRAW_FLAGS_INDEX_16;
      }

      crude_gfx_model_renderer_resources_manager_gltf_write_mesh_material_( manager, mesh_cpu, gltf, gltf_mesh_primitive->material, images_offset, samplers_offset );

      mesh_cpu->indices_count = gltf_indices_accessor->count;
      mesh_cpu->gpu_mesh_global_index = crude_gfX_get_gpu_mesh_global_index_( manager->total_meshes_count, mesh_index );
      mesh_cpu->default_bounding_sphere.x = XMVectorGetX( bounding_center );
      mesh_cpu->default_bounding_sphere.y = XMVectorGetY( bounding_center );
      mesh_cpu->default_bounding_sphere.z = XMVectorGetZ( bounding_center );
      mesh_cpu->default_bounding_sphere.w = bounding_radius;
      mesh_cpu->flags |= flags;
      ++mesh_index;
    }
  }

  if ( manager->gpu->mesh_shaders_extension_present )
  {
    crude_gfx_meshlet                                     *local_meshlets;
    crude_gfx_vertex                                      *local_meshlets_vertices;
    XMFLOAT3                                              *local_meshlets_vertices_positions;
    crude_gfx_vertex_joint                                *local_meshlets_vertices_joints;
    uint32                                                *local_meshlets_vertices_indices;
    uint8                                                 *local_meshlets_triangles_indices;
    uint64                                                 local_meshlets_vertices_offset, local_meshlets_vertices_count;
    uint32                                                 local_meshlets_offset, local_meshlets_vertices_indices_offset, local_meshlets_triangles_indices_offset;

    local_meshlets_vertices_count = 0u;
  
    for ( uint32 i = 0; i < gltf->meshes_count; ++i )
    {
      cgltf_mesh                                          *gltf_mesh;
      
      gltf_mesh = &gltf->meshes[ i ];
      for ( uint32 gltf_primitive_index = 0; gltf_primitive_index < gltf_mesh->primitives_count; ++gltf_primitive_index )
      {
        local_meshlets_vertices_count += gltf_mesh->primitives[ gltf_primitive_index ].attributes[ 0 ].data->count;
      }
    }

    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( local_meshlets, 0, crude_heap_allocator_pack( manager->allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( local_meshlets_vertices_indices, 0, crude_heap_allocator_pack( manager->allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( local_meshlets_triangles_indices, 0, crude_heap_allocator_pack( manager->allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( local_meshlets_vertices, local_meshlets_vertices_count, crude_heap_allocator_pack( manager->allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( local_meshlets_vertices_positions, local_meshlets_vertices_count, crude_heap_allocator_pack( manager->allocator ) );
    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( local_meshlets_vertices_joints, local_meshlets_vertices_count, crude_heap_allocator_pack( manager->allocator ) );
  
    local_meshlets_offset = 0u;
    local_meshlets_vertices_indices_offset = 0u;
    local_meshlets_triangles_indices_offset = 0u;
    local_meshlets_vertices_offset = 0u;
    mesh_index = 0u;

    for ( uint32 i = 0; i < gltf->meshes_count; ++i )
    {
      cgltf_mesh                                          *gltf_mesh;
      
      gltf_mesh = &gltf->meshes[ i ];

      for ( uint32 gltf_primitive_index = 0; gltf_primitive_index < gltf_mesh->primitives_count; ++gltf_primitive_index )
      {
        crude_gfx_meshlet const                           *last_local_meshlet;
        cgltf_primitive                                   *gltf_mesh_primitive;
        uint32                                            *primitive_indices;
        meshopt_Meshlet                                   *primitive_meshlets;
        uint64                                             primitive_local_max_meshlets, primitive_local_meshletes_count, primitive_local_meshletes_offset;
        uint32                                             primitive_vertices_count, primitive_indices_count;
      
        gltf_mesh_primitive = &gltf_mesh->primitives[ gltf_primitive_index ];

        primitive_vertices_count = gltf_mesh->primitives[ gltf_primitive_index ].attributes[ 0 ].data->count;
        primitive_indices_count = gltf_mesh->primitives[ gltf_primitive_index ].indices->count;
      
        CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( primitive_indices, primitive_indices_count, crude_heap_allocator_pack( manager->allocator ) );
      
        crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_vertices_(
          gltf_mesh_primitive,
          local_meshlets_vertices + local_meshlets_vertices_offset,
          local_meshlets_vertices_positions + local_meshlets_vertices_offset,
          local_meshlets_vertices_joints + local_meshlets_vertices_offset );

        crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_indices_(
          gltf_mesh_primitive,
          primitive_indices );

        /* Build meshlets*/
        primitive_local_max_meshlets = meshopt_buildMeshletsBound(
          primitive_indices_count,
          CRUDE_GFX_MESHLET_MAX_VERTICES,
          CRUDE_GFX_MESHLET_MAX_TRIANGLES );
      
        CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( primitive_meshlets, primitive_local_max_meshlets, crude_heap_allocator_pack( manager->allocator ) );

        CRUDE_ARRAY_SET_LENGTH( local_meshlets_vertices_indices, local_meshlets_vertices_indices_offset + primitive_local_max_meshlets * CRUDE_GFX_MESHLET_MAX_VERTICES );
        CRUDE_ARRAY_SET_LENGTH( local_meshlets_triangles_indices, local_meshlets_triangles_indices_offset + primitive_local_max_meshlets * CRUDE_GFX_MESHLET_MAX_TRIANGLES * 3 );
        
        /* Build meshlets */
        primitive_local_meshletes_count = meshopt_buildMeshlets(
          primitive_meshlets,
          local_meshlets_vertices_indices + local_meshlets_vertices_indices_offset,
          local_meshlets_triangles_indices + local_meshlets_triangles_indices_offset,
          primitive_indices, primitive_indices_count, 
          &local_meshlets_vertices_positions[ local_meshlets_vertices_offset ].x,
          primitive_vertices_count,
          sizeof( crude_gfx_vertex_position ),
          CRUDE_GFX_MESHLET_MAX_VERTICES, CRUDE_GFX_MESHLET_MAX_TRIANGLES, CRUDE_GFX_MESHLET_CONE_WEIGHT );
      
        CRUDE_ARRAY_SET_LENGTH( local_meshlets, local_meshlets_offset + primitive_local_meshletes_count );

        /* Optimize meshlets */
        for ( uint32 meshopt_meshlet_index = 0; meshopt_meshlet_index < primitive_local_meshletes_count; ++meshopt_meshlet_index )
        {
          meshopt_Meshlet const                           *meshopt_local_meshlet;
          
          meshopt_local_meshlet = &primitive_meshlets[ meshopt_meshlet_index ];

          CRUDE_ASSERT( meshopt_local_meshlet->vertex_count <= CRUDE_GFX_MESHLET_MAX_VERTICES );
          CRUDE_ASSERT( meshopt_local_meshlet->triangle_count <= CRUDE_GFX_MESHLET_MAX_TRIANGLES );

          meshopt_optimizeMeshlet(
            local_meshlets_vertices_indices + local_meshlets_vertices_indices_offset + meshopt_local_meshlet->vertex_offset,
            local_meshlets_triangles_indices + local_meshlets_triangles_indices_offset + meshopt_local_meshlet->triangle_offset,
            meshopt_local_meshlet->triangle_count, meshopt_local_meshlet->vertex_count );
        }

        /* Fill meshlets */
        for ( uint32 meshopt_meshlet_index = 0; meshopt_meshlet_index < primitive_local_meshletes_count; ++meshopt_meshlet_index )
        {
          crude_gfx_meshlet                               *new_meshlet;
          meshopt_Meshlet const                           *meshopt_local_meshlet;
          meshopt_Bounds                                   meshopt_meshlet_bounds;

          meshopt_local_meshlet = &primitive_meshlets[ meshopt_meshlet_index ];

          meshopt_meshlet_bounds = meshopt_computeMeshletBounds(
            local_meshlets_vertices_indices + local_meshlets_vertices_indices_offset + meshopt_local_meshlet->vertex_offset,
            local_meshlets_triangles_indices + local_meshlets_triangles_indices_offset + meshopt_local_meshlet->triangle_offset,
            meshopt_local_meshlet->triangle_count,
            &local_meshlets_vertices_positions[ local_meshlets_vertices_offset ].x,
            primitive_vertices_count,
            sizeof( crude_gfx_vertex_position ) );

          new_meshlet = &local_meshlets[ meshopt_meshlet_index + local_meshlets_offset ];
          new_meshlet->vertices_offset = local_meshlets_vertices_indices_offset + meshopt_local_meshlet->vertex_offset;
          new_meshlet->triangles_offset = local_meshlets_triangles_indices_offset + meshopt_local_meshlet->triangle_offset;
          new_meshlet->vertices_count = meshopt_local_meshlet->vertex_count;
          new_meshlet->triangles_count = meshopt_local_meshlet->triangle_count;
          new_meshlet->mesh_index = model_renderer_resouces->meshes[ mesh_index ].gpu_mesh_global_index;
          new_meshlet->center.x = meshopt_meshlet_bounds.center[ 0 ];
          new_meshlet->center.y = meshopt_meshlet_bounds.center[ 1 ];
          new_meshlet->center.z = meshopt_meshlet_bounds.center[ 2 ];
          new_meshlet->radius = meshopt_meshlet_bounds.radius;
          new_meshlet->cone_axis[ 0 ] = meshopt_meshlet_bounds.cone_axis_s8[ 0 ];
          new_meshlet->cone_axis[ 1 ] = meshopt_meshlet_bounds.cone_axis_s8[ 1 ];
          new_meshlet->cone_axis[ 2 ] = meshopt_meshlet_bounds.cone_axis_s8[ 2 ];
          new_meshlet->cone_cutoff = meshopt_meshlet_bounds.cone_cutoff_s8;
        }
        
        last_local_meshlet = &local_meshlets[ local_meshlets_offset + primitive_local_meshletes_count - 1 ];
        
        /* Fill meshelets data to mesh */
        model_renderer_resouces->meshes[ mesh_index ].meshlets_count = primitive_local_meshletes_count;
        model_renderer_resouces->meshes[ mesh_index ].meshlets_offset = local_meshlets_offset + manager->total_meshlets_count;

        /* We fill offset for each primitive */
        for ( uint32 i = local_meshlets_vertices_indices_offset; i < last_local_meshlet->vertices_offset + last_local_meshlet->vertices_count; ++i )
        {
          local_meshlets_vertices_indices[ i ] += local_meshlets_vertices_offset + manager->total_meshlets_vertices_count;
        }

        local_meshlets_vertices_indices_offset = last_local_meshlet->vertices_offset + last_local_meshlet->vertices_count;
        local_meshlets_triangles_indices_offset = last_local_meshlet->triangles_offset + 3u * last_local_meshlet->triangles_count;
        local_meshlets_offset += primitive_local_meshletes_count;
        local_meshlets_vertices_offset += primitive_vertices_count;
      
        CRUDE_ARRAY_DEINITIALIZE( primitive_meshlets );
        CRUDE_ARRAY_DEINITIALIZE( primitive_indices );

        ++mesh_index;
      }
    }
    
    /* Create indices buffer for meshes (so we still can use meshlets vertices ) */
    {
      mesh_index = 0u;
      for ( uint32 gltf_mesh_index = 0; gltf_mesh_index < gltf->meshes_count; ++gltf_mesh_index )
      {
        cgltf_mesh                                        *gltf_mesh;

        gltf_mesh = &gltf->meshes[ gltf_mesh_index ];

        for ( uint32 gltf_primitive_index = 0; gltf_primitive_index < gltf_mesh->primitives_count; ++gltf_primitive_index )
        {
          crude_gfx_mesh_cpu                                  *mesh_cpu;
          uint32                                              *mesh_indices;
          crude_gfx_memory_allocation                          cpu_allocation;
          uint64                                               mesh_indices_count;
          
          mesh_cpu = &model_renderer_resouces->meshes[ mesh_index ];

          mesh_indices_count = 0u;
          for ( uint32 local_meshlet_index = 0; local_meshlet_index < mesh_cpu->meshlets_count; ++local_meshlet_index )
          {
            mesh_indices_count += 3 * local_meshlets[ local_meshlet_index ].triangles_count;
          }

          CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( mesh_indices, mesh_indices_count, crude_heap_allocator_pack( manager->allocator ) );

          mesh_indices_count = 0u;
          for ( uint32 local_meshlet_index = 0; local_meshlet_index < mesh_cpu->meshlets_count; ++local_meshlet_index )
          {
            crude_gfx_meshlet const                       *meshlet;
            
            meshlet = &local_meshlets[ local_meshlet_index ];
            for ( uint32 t = 0; t < 3 * meshlet->triangles_count; ++t )
            {
              uint32                                       vertex_index, triangle_index;

              triangle_index = local_meshlets_triangles_indices[ t + meshlet->triangles_offset ];
              vertex_index = local_meshlets_vertices_indices[ triangle_index + meshlet->vertices_offset ];
              mesh_indices[ mesh_indices_count++ ] = vertex_index;
            }
          }
          ++mesh_index;
          
          cpu_allocation = crude_gfx_memory_allocate_cpu_gpu_copy(
            manager->gpu,
            mesh_indices,
            sizeof( mesh_indices[ 0 ] ) * mesh_indices_count, VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR  );

          /* Queue cpu memory destroy */
          crude_gfx_memory_deallocate( manager->gpu, cpu_allocation );

          mesh_cpu->index_hga = crude_gfx_memory_allocate_with_name(
            manager->gpu,
            sizeof( mesh_indices[ 0 ] ) * mesh_indices_count,
            CRUDE_GFX_MEMORY_TYPE_GPU,
            "mseh_index_buffer", VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR  );

          crude_gfx_cmd_memory_copy( immediate_transfer_cmd_buffer, cpu_allocation, mesh_cpu->index_hga, 0u, 0u );

          CRUDE_ARRAY_PUSH( manager->indices_buffers, mesh_cpu->index_hga );

          CRUDE_ARRAY_DEINITIALIZE( mesh_indices );
        }
      }
    }

    for ( uint32 i = 0; i < local_meshlets_offset; ++i )
    {
      local_meshlets[ i ].vertices_offset += manager->total_meshlets_vertices_indices_count;
      local_meshlets[ i ].triangles_offset += manager->total_meshlets_triangles_indices_count;
    }

    manager->total_meshlets_count += local_meshlets_offset;
    manager->total_meshlets_vertices_count += local_meshlets_vertices_offset;
    manager->total_meshlets_vertices_indices_count += local_meshlets_vertices_indices_offset;
    manager->total_meshlets_triangles_indices_count += local_meshlets_triangles_indices_offset;
    
    manager->meshlets_hga = crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy(
      manager->async_loader,
      crude_gfx_memory_allocate_cpu_gpu_copy(
        manager->gpu,
        local_meshlets,
        sizeof( local_meshlets[ 0 ] ) * local_meshlets_offset, 0 ),
      crude_gfx_memory_allocate_with_name(
        manager->gpu,
        sizeof( local_meshlets[ 0 ] ) * manager->total_meshlets_count,
        CRUDE_GFX_MEMORY_TYPE_GPU,
        "meshlets_hga", 0 ),
      manager->meshlets_hga );

    manager->meshlets_triangles_indices_hga = crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy(
      manager->async_loader,
      crude_gfx_memory_allocate_cpu_gpu_copy(
        manager->gpu,
        local_meshlets_triangles_indices,
        sizeof( local_meshlets_triangles_indices[ 0 ] ) * local_meshlets_triangles_indices_offset, 0 ),
      crude_gfx_memory_allocate_with_name(
        manager->gpu,
        sizeof( local_meshlets_triangles_indices[ 0 ] ) * manager->total_meshlets_triangles_indices_count,
        CRUDE_GFX_MEMORY_TYPE_GPU,
        "meshlets_triangles_indices_hga", 0 ),
      manager->meshlets_triangles_indices_hga );
      
    manager->meshlets_vertices_indices_hga = crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy(
      manager->async_loader,
      crude_gfx_memory_allocate_cpu_gpu_copy(
        manager->gpu,
        local_meshlets_vertices_indices,
        sizeof( local_meshlets_vertices_indices[ 0 ] ) * local_meshlets_vertices_indices_offset, 0 ),
      crude_gfx_memory_allocate_with_name(
        manager->gpu,
        sizeof( local_meshlets_vertices_indices[ 0 ] ) * manager->total_meshlets_vertices_indices_count,
        CRUDE_GFX_MEMORY_TYPE_GPU,
        "meshlets_vertices_indices_hga", 0 ),
      manager->meshlets_vertices_indices_hga );
      
    manager->meshlets_vertices_hga = crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy(
      manager->async_loader,
      crude_gfx_memory_allocate_cpu_gpu_copy(
        manager->gpu,
        local_meshlets_vertices,
        sizeof( local_meshlets_vertices[ 0 ] ) * local_meshlets_vertices_offset, 0 ),
      crude_gfx_memory_allocate_with_name(
        manager->gpu,
        sizeof( local_meshlets_vertices[ 0 ] ) * manager->total_meshlets_vertices_count,
        CRUDE_GFX_MEMORY_TYPE_GPU,
        "meshlets_vertices_hga", 0 ),
      manager->meshlets_vertices_hga );
      
    /* Wee need it for blas creation, so create and copy buffer right now (no async ) */
    {
      crude_gfx_memory_allocation                          gpu_allocation, cpu_allocation;

      cpu_allocation = crude_gfx_memory_allocate_cpu_gpu_copy(
        manager->gpu,
        local_meshlets_vertices_positions,
        sizeof( local_meshlets_vertices_positions[ 0 ] ) * local_meshlets_vertices_offset, VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR  );
      
      /* Queue cpu memory destroy */
      crude_gfx_memory_deallocate( manager->gpu, cpu_allocation );

      gpu_allocation = crude_gfx_memory_allocate_with_name(
        manager->gpu,
        sizeof( local_meshlets_vertices_positions[ 0 ] ) * manager->total_meshlets_vertices_count,
        CRUDE_GFX_MEMORY_TYPE_GPU,
        "meshlets_vertices_positions_hga", VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR  );

      if ( manager->meshlets_vertices_positions_hga.size )
      {
        crude_gfx_cmd_memory_copy( immediate_transfer_cmd_buffer, manager->meshlets_vertices_positions_hga, gpu_allocation, 0u, 0u );
      }
      crude_gfx_cmd_memory_copy( immediate_transfer_cmd_buffer, cpu_allocation, gpu_allocation, 0u, manager->meshlets_vertices_positions_hga.size );

      manager->meshlets_vertices_positions_hga = gpu_allocation;
    }
      
    manager->meshlets_vertices_joints_hga = crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy(
      manager->async_loader,
      crude_gfx_memory_allocate_cpu_gpu_copy(
        manager->gpu,
        local_meshlets_vertices_joints,
        sizeof( local_meshlets_vertices_joints[ 0 ] ) * local_meshlets_vertices_offset, 0 ),
      crude_gfx_memory_allocate_with_name(
        manager->gpu,
        sizeof( local_meshlets_vertices_joints[ 0 ] ) * manager->total_meshlets_vertices_count,
        CRUDE_GFX_MEMORY_TYPE_GPU,
        "meshlets_vertices_joints_hga", 0 ),
      manager->meshlets_vertices_joints_hga );
    
    CRUDE_ARRAY_DEINITIALIZE( local_meshlets );
    CRUDE_ARRAY_DEINITIALIZE( local_meshlets_vertices_indices );
    CRUDE_ARRAY_DEINITIALIZE( local_meshlets_triangles_indices );
    CRUDE_ARRAY_DEINITIALIZE( local_meshlets_vertices );
    CRUDE_ARRAY_DEINITIALIZE( local_meshlets_vertices_positions );
    CRUDE_ARRAY_DEINITIALIZE( local_meshlets_vertices_joints );
  }
  
  manager->total_meshes_count += CRUDE_ARRAY_LENGTH( model_renderer_resouces->meshes );

  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( meshes_draws, CRUDE_ARRAY_LENGTH( model_renderer_resouces->meshes ), crude_heap_allocator_pack( manager->allocator ) );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model_renderer_resouces->meshes ); ++i )
  {
    crude_gfx_mesh_cpu_to_mesh_draw_gpu( manager->gpu, &model_renderer_resouces->meshes[ i ], &meshes_draws[ i ] );
  }
  
  manager->meshes_draws_hga = crude_gfx_asynchronous_loader_request_buffer_reallocate_and_copy(
    manager->async_loader,
    crude_gfx_memory_allocate_cpu_gpu_copy(
      manager->gpu,
      meshes_draws,
      sizeof( meshes_draws[ 0 ] ) * CRUDE_ARRAY_LENGTH( model_renderer_resouces->meshes ), 0 ),
    crude_gfx_memory_allocate_with_name(
      manager->gpu,
      sizeof( meshes_draws[ 0 ] ) * manager->total_meshes_count,
      CRUDE_GFX_MEMORY_TYPE_GPU,
      "meshes_draws_hga", 0 ),
    manager->meshes_draws_hga );

  CRUDE_ARRAY_DEINITIALIZE( meshes_draws );
  
  crude_gfx_submit_immediate( immediate_transfer_cmd_buffer );
}

void
crude_gfx_model_renderer_resources_manager_get_cgltf_node_transform
(
  _In_ cgltf_node const                                   *gltf_node,
  _Out_ crude_transform                                   *transform
)
{
  if ( gltf_node->has_translation )
  {
    XMStoreFloat3( &transform->translation, XMVectorSet( gltf_node->translation[ 0 ], gltf_node->translation[ 1 ], gltf_node->translation[ 2 ], 1 ));
  }
  else
  {
    XMStoreFloat3( &transform->translation, XMVectorZero( ) );
  }
  
  if ( gltf_node->has_scale )
  {
    XMStoreFloat3( &transform->scale, XMVectorSet( gltf_node->scale[ 0 ], gltf_node->scale[ 1 ], gltf_node->scale[ 2 ], 1 ));
  }
  else
  {
    XMStoreFloat3( &transform->scale, XMVectorReplicate( 1.f ) );
  }
  
  if ( gltf_node->has_rotation )
  {
    XMStoreFloat4( &transform->rotation, XMVectorSet( gltf_node->rotation[ 0 ], gltf_node->rotation[ 1 ], gltf_node->rotation[ 2 ], gltf_node->rotation[ 3 ] ) );
  }
  else
  {
    XMStoreFloat4( &transform->rotation, XMQuaternionIdentity( ) );
  }
  
  if ( gltf_node->has_matrix )
  {
    XMVECTOR                                             decompose_scale;
    XMVECTOR                                             decompose_rotation_quat;
    XMVECTOR                                             decompose_translation;
    XMFLOAT4X4                                           gltf_node_matrix;
  
    CRUDE_ASSERT( !gltf_node->has_translation );
    CRUDE_ASSERT( !gltf_node->has_scale );
    CRUDE_ASSERT( !gltf_node->has_rotation );
    gltf_node_matrix = XMFLOAT4X4{ gltf_node->matrix };
    crude_transform_decompose( transform, XMLoadFloat4x4( &gltf_node_matrix ) );
  }
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_nodes_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources,
  _In_ cgltf_data                                         *gltf,
  _In_ uint32                                             *gltf_mesh_index_to_mesh_primitive_index
)
{
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( model_renderer_resources->nodes, gltf->nodes_count, crude_heap_allocator_pack( manager->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( model_renderer_resources->default_nodes_transforms, gltf->nodes_count, crude_heap_allocator_pack( manager->allocator ) );
  
  for ( uint32 i = 0; i < gltf->nodes_count; ++i )
  {
    cgltf_node                                            *gltf_node;
    crude_transform                                       *transform;
    crude_gfx_node                                        *node;

    gltf_node = &gltf->nodes[ i ];
    node = &model_renderer_resources->nodes[ i ];
    transform = &model_renderer_resources->default_nodes_transforms[ i ];
    
    crude_string_copy( node->name, gltf_node->name ? gltf_node->name : "None", sizeof( node->name ) );

    if ( gltf_node->skin )
    {
      node->skin = cgltf_skin_index( gltf, gltf_node->skin );
    }
    else
    {
      node->skin = -1;
    }

    if ( gltf_node->parent )
    {
      node->parent = cgltf_node_index( gltf, gltf_node->parent );
    }
    else
    {
      node->parent = -1;
    }
    
    if ( gltf_node->children_count )
    {
      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->childrens, gltf_node->children_count, crude_heap_allocator_pack( manager->allocator ) );
      for ( uint32 k = 0; k < gltf_node->children_count; ++k )
      {
        node->childrens[ k ] = cgltf_node_index( gltf, gltf_node->children[ k ] );
      }
    }
    else
    {
      node->childrens = NULL;
    }

    if ( gltf_node->mesh )
    {
      uint32                                               mesh_index_offset;

      mesh_index_offset = gltf_mesh_index_to_mesh_primitive_index[ cgltf_mesh_index( gltf, gltf_node->mesh ) ];
      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->meshes, gltf_node->mesh->primitives_count, crude_heap_allocator_pack( manager->allocator )  );

      for ( uint32 pi = 0; pi < gltf_node->mesh->primitives_count; ++pi )
      {
        crude_gfx_mesh_cpu                                *mesh;

        node->meshes[ pi ] = mesh_index_offset + pi;
      }
    }
    else
    {
      node->meshes = NULL;
    }
    
    /* Joints */
    if ( gltf_node->mesh && gltf_node->skin )
    {
      for ( uint32 pi = 0; pi < gltf_node->mesh->primitives_count; ++pi )
      {
        crude_gfx_aabb_cpu                                *joints_aabbs;
        crude_gfx_skin                                    *skin;
        cgltf_primitive                                   *gltf_primitive;
        uint8                                             *primitive_positions_data;
        uint8                                             *primitive_joints_data;
        uint32                                             primitive_positions_stride;
        uint32                                             primitive_joints_stride;
        cgltf_component_type                               primitive_joints_type;
        uint32                                             primitive_vertices_count;
          
        skin = &model_renderer_resources->skins[ node->skin ];

        gltf_primitive = &gltf_node->mesh->primitives[ pi ];
          
        primitive_positions_data = primitive_joints_data = NULL;

        primitive_joints_stride = primitive_positions_stride = 0;

        primitive_vertices_count = gltf_primitive->attributes[ 0 ].data->count;

        CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( joints_aabbs, CRUDE_ARRAY_LENGTH( skin->joints ), crude_heap_allocator_pack( manager->allocator ) );

        for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( joints_aabbs ); ++i )
        {
          joints_aabbs[ i ].max.x = FLT_MAX;
        }

        for ( uint32 i = 0; i < gltf_primitive->attributes_count; ++i )
        {
          cgltf_attribute *attribute = &gltf_primitive->attributes[ i ];
          CRUDE_ASSERT( primitive_vertices_count == attribute->data->count );

          uint8 *attribute_data = CRUDE_STATIC_CAST( uint8*, attribute->data->buffer_view->buffer->data ) + attribute->data->buffer_view->offset + attribute->data->offset;
          switch ( attribute->type )
          {
          case cgltf_attribute_type_position:
          {
            CRUDE_ASSERT( attribute->data->type == cgltf_type_vec3 );
            primitive_positions_stride = attribute->data->stride;
            primitive_positions_data = CRUDE_CAST( uint8*, attribute_data );
            break;
          }
          case cgltf_attribute_type_joints:
          {
            CRUDE_ASSERT( attribute->data->type == cgltf_type_vec4 );

            primitive_joints_stride = attribute->data->stride;
            primitive_joints_data = CRUDE_CAST( uint8*, attribute_data );
            primitive_joints_type = attribute->data->component_type;
            break;
          }
          }
        }
        
        CRUDE_ASSERT( primitive_positions_data );

        for ( uint32 i = 0; i < primitive_vertices_count; ++i )
        {
          XMFLOAT3                                      *vertex_position;

          vertex_position = CRUDE_CAST( XMFLOAT3*, primitive_positions_data );
          primitive_positions_data += primitive_positions_stride;

          if ( primitive_joints_data )
          {
            for ( uint32 k = 0; k < 4; ++k )
            {
              uint32                                     joint_index;
              
              if ( primitive_joints_type == cgltf_component_type_r_8u )
              {
                joint_index = primitive_joints_data[ k ];
              }
              else
              {
                CRUDE_ASSERT( primitive_joints_type == cgltf_component_type_r_16u );
                joint_index = CRUDE_CAST( uint16*, primitive_joints_data )[ k ];
              }

              /* !TODO i fucking hate this implementation */
              if ( joints_aabbs[ joint_index ].max.x == FLT_MAX )
              {
                joints_aabbs[ joint_index ].max.x = vertex_position->x;
                joints_aabbs[ joint_index ].max.y = vertex_position->y;
                joints_aabbs[ joint_index ].max.z = vertex_position->z;
                joints_aabbs[ joint_index ].min.x = vertex_position->x;
                joints_aabbs[ joint_index ].min.y = vertex_position->y;
                joints_aabbs[ joint_index ].min.z = vertex_position->z;
              }
              else
              {
                joints_aabbs[ joint_index ].max.x = CRUDE_MAX( joints_aabbs[ joint_index ].max.x, vertex_position->x );
                joints_aabbs[ joint_index ].max.y = CRUDE_MAX( joints_aabbs[ joint_index ].max.y, vertex_position->y );
                joints_aabbs[ joint_index ].max.z = CRUDE_MAX( joints_aabbs[ joint_index ].max.z, vertex_position->z );
                joints_aabbs[ joint_index ].min.x = CRUDE_MIN( joints_aabbs[ joint_index ].min.x, vertex_position->x );
                joints_aabbs[ joint_index ].min.y = CRUDE_MIN( joints_aabbs[ joint_index ].min.y, vertex_position->y );
                joints_aabbs[ joint_index ].min.z = CRUDE_MIN( joints_aabbs[ joint_index ].min.z, vertex_position->z );
              }
            }
            
            primitive_joints_data += primitive_joints_stride;
          }
        }

        CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->affected_joints, 0, crude_heap_allocator_pack( manager->allocator ) );
        CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( node->affected_joints_local_aabb, 0, crude_heap_allocator_pack( manager->allocator ) );
        for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( joints_aabbs ); ++i )
        {
          if ( joints_aabbs[ i ].max.x != FLT_MAX )
          {
            CRUDE_ARRAY_PUSH( node->affected_joints, i );
            CRUDE_ARRAY_PUSH( node->affected_joints_local_aabb, joints_aabbs[ i ] );
          }
        }

        CRUDE_ARRAY_DEINITIALIZE( joints_aabbs );
      }
    }
    else
    {
      node->affected_joints = NULL;
      node->affected_joints_local_aabb = NULL;
    }

    crude_gfx_model_renderer_resources_manager_get_cgltf_node_transform( gltf_node, transform );
  }
}

void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_vertices_
(
  _In_ cgltf_primitive                                    *primitive,
  _Out_ crude_gfx_vertex                                  *vertices,
  _Out_ crude_gfx_vertex_position                         *vertices_positions,
  _Out_ crude_gfx_vertex_joint                            *vertices_joints
)
{
  uint8                                                   *primitive_tangents_data;
  uint8                                                   *primitive_positions_data;
  uint8                                                   *primitive_normals_data;
  uint8                                                   *primitive_texcoords_data;
  uint8                                                   *primitive_joints_data;
  uint8                                                   *primitive_weights_data;
  uint32                                                   primitive_positions_stride;
  uint32                                                   primitive_tangents_stride;
  uint32                                                   primitive_texcoords_stride;
  uint32                                                   primitive_joints_stride;
  uint32                                                   primitive_weights_stride;
  uint32                                                   primitive_normals_stride;
  cgltf_component_type                                     primitive_joints_type;
  uint32                                                   meshlet_vertices_count;
  
  primitive_tangents_data = primitive_positions_data = primitive_normals_data = NULL;
  primitive_texcoords_data = primitive_weights_data = primitive_joints_data = NULL;

  primitive_joints_stride = primitive_positions_stride = 0;
  primitive_normals_stride = primitive_weights_stride = 0;

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
      primitive_positions_stride = attribute->data->stride;
      primitive_positions_data = CRUDE_CAST( uint8*, attribute_data );
      break;
    }
    case cgltf_attribute_type_tangent:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec4 );
      primitive_tangents_data = CRUDE_CAST( uint8*, attribute_data );
      primitive_tangents_stride = attribute->data->stride;
      break;
    }
    case cgltf_attribute_type_normal:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec3 );
      primitive_normals_stride = attribute->data->stride;
      primitive_normals_data = CRUDE_CAST( uint8*, attribute_data );
      break;
    }
    case cgltf_attribute_type_texcoord:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec2 );
      primitive_texcoords_data = CRUDE_CAST( uint8*, attribute_data );
      primitive_texcoords_stride = attribute->data->stride;
      break;
    }
    case cgltf_attribute_type_joints:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec4 );

      primitive_joints_stride = attribute->data->stride;
      primitive_joints_data = CRUDE_CAST( uint8*, attribute_data );
      primitive_joints_type = attribute->data->component_type;
      break;
    }
    case cgltf_attribute_type_weights:
    {
      CRUDE_ASSERT( attribute->data->type == cgltf_type_vec4 );
      primitive_weights_data = CRUDE_CAST( uint8*, attribute_data );
      primitive_weights_stride = attribute->data->stride;
      break;
    }
    }
  }
  
  CRUDE_ASSERT( primitive_positions_data );

  for ( uint32 i = 0; i < meshlet_vertices_count; ++i )
  {
    crude_gfx_vertex *vertex = &vertices[ i ];
    crude_gfx_vertex_position *vertex_position = &vertices_positions[ i ];
    crude_gfx_vertex_joint *vertex_joint = &vertices_joints[ i ];

    *vertex_position = *CRUDE_CAST( XMFLOAT3*, primitive_positions_data );

    primitive_positions_data += primitive_positions_stride;

    if ( primitive_normals_data )
    {
      XMFLOAT3                                            *normal;
      
      normal = CRUDE_CAST( XMFLOAT3*, primitive_normals_data );

      vertex->normal[ 0 ] = ( normal->x + 1.0f ) * 127.0f;
      vertex->normal[ 1 ] = ( normal->y + 1.0f ) * 127.0f;
      vertex->normal[ 2 ] = ( normal->z + 1.0f ) * 127.0f;

      primitive_normals_data += primitive_normals_stride;
    }

    if ( primitive_tangents_data  )
    {
      XMFLOAT4                                            *tangent;
      
      tangent = CRUDE_CAST( XMFLOAT4*, primitive_tangents_data );

      vertex->tangent[ 0 ] = ( tangent->x + 1.0f ) * 127.0f;
      vertex->tangent[ 1 ] = ( tangent->y + 1.0f ) * 127.0f;
      vertex->tangent[ 2 ] = ( tangent->z + 1.0f ) * 127.0f;
      vertex->tangent[ 3 ] = ( tangent->w + 1.0f ) * 127.0f;

      primitive_tangents_data += primitive_tangents_stride;
    }

    if ( primitive_texcoords_data )
    {
      XMFLOAT2                                            *texcoord;
      
      texcoord = CRUDE_CAST( XMFLOAT2*, primitive_texcoords_data );

      vertex->texcoords[ 0 ] = meshopt_quantizeHalf( texcoord->x );
      vertex->texcoords[ 1 ] = meshopt_quantizeHalf( texcoord->y );

      primitive_texcoords_data += primitive_texcoords_stride;
    }

    if ( primitive_weights_data )
    {
      XMFLOAT4                                            *weight;
      
      weight = CRUDE_CAST( XMFLOAT4*, primitive_weights_data );

      vertex_joint->joint_weights = *weight;

      primitive_weights_data += primitive_weights_stride;
    }

    if ( primitive_joints_data )
    {
      if ( primitive_joints_type == cgltf_component_type_r_8u  )
      {
        vertex_joint->joint_indices.x = primitive_joints_data[ 0 ];
        vertex_joint->joint_indices.y = primitive_joints_data[ 1 ];
        vertex_joint->joint_indices.z = primitive_joints_data[ 2 ];
        vertex_joint->joint_indices.w = primitive_joints_data[ 3 ];
      }
      else if ( primitive_joints_type == cgltf_component_type_r_16u )
      {
        vertex_joint->joint_indices.x = CRUDE_CAST( uint16*, primitive_joints_data )[ 0 ];
        vertex_joint->joint_indices.y = CRUDE_CAST( uint16*, primitive_joints_data )[ 1 ];
        vertex_joint->joint_indices.z = CRUDE_CAST( uint16*, primitive_joints_data )[ 2 ];
        vertex_joint->joint_indices.w = CRUDE_CAST( uint16*, primitive_joints_data )[ 3 ];
      }
      
      primitive_joints_data += primitive_joints_stride;
    }

    if ( !primitive_weights_data && !primitive_joints_data )
    {
      vertex_joint->joint_weights = CRUDE_COMPOUNT( XMFLOAT4, { 1, 0, 0, 0 } );
      vertex_joint->joint_indices = CRUDE_COMPOUNT_EMPTY( XMFLOAT4 );
    }
  }
}

static void
crude_gfx_model_renderer_resources_manager_gltf_load_meshlet_indices_
(
  _In_ cgltf_primitive                                    *primitive,
  _In_ uint32                                             *indices
)
{
  uint32                                                   meshlet_vertices_indices_count;
  uint8                                                   *buffer_data;

  meshlet_vertices_indices_count = primitive->indices->count;
  buffer_data = CRUDE_CAST( uint8*, primitive->indices->buffer_view->buffer->data ) + primitive->indices->buffer_view->offset + primitive->indices->offset;
  
  CRUDE_ASSERT( primitive->indices->type == cgltf_type_scalar );
  //CRUDE_ASSERT( primitive->indices->component_type == cgltf_component_type_r_16u ); // change ray tracing index property in geometry 

  if ( primitive->indices->component_type == cgltf_component_type_r_16u )
  {
    uint16 *primitive_indices = CRUDE_CAST( uint16*, buffer_data );
    for ( uint32 i = 0; i < meshlet_vertices_indices_count; ++i )
    {
      indices[ i ] = primitive_indices[ i ];
    }
  }
  else if ( primitive->indices->component_type == cgltf_component_type_r_32u )
  {
    uint32 *primitive_indices = CRUDE_CAST( uint32*, buffer_data );
    for ( uint32 i = 0; i < meshlet_vertices_indices_count; ++i )
    {
      indices[ i ] = primitive_indices[ i ];
    }
  }
  else
  {
    CRUDE_ASSERT( false );
  }
}


static void
crude_gfx_model_renderer_resources_manager_load_skins_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources,
  _In_ cgltf_data                                         *gltf
)
{
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( model_renderer_resources->skins, gltf->skins_count, crude_heap_allocator_pack( manager->allocator ) );

  for ( uint64 skin_index = 0; skin_index < gltf->skins_count; ++skin_index )
  {
    cgltf_skin                                            *gltf_skin;
    crude_gfx_skin                                       *skin;

    gltf_skin = &gltf->skins[ skin_index ];
    skin = &model_renderer_resources->skins[ skin_index ];
    
    if ( gltf_skin->inverse_bind_matrices )
    {
      uint8                                               *inverse_bind_matrix_data;

      inverse_bind_matrix_data = CRUDE_CAST( uint8*, gltf_skin->inverse_bind_matrices->buffer_view->buffer->data ) + gltf_skin->inverse_bind_matrices->buffer_view->offset;

      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( skin->inverse_bind_matrices, gltf_skin->inverse_bind_matrices->count, crude_heap_allocator_pack( manager->allocator ) );
      for ( uint32 i = 0; i < gltf_skin->inverse_bind_matrices->count; ++i )
      {
        skin->inverse_bind_matrices[ i ] = *CRUDE_CAST( XMFLOAT4X4*, inverse_bind_matrix_data );
        inverse_bind_matrix_data += gltf_skin->inverse_bind_matrices->stride;
      }
    }
    else
    {
      skin->inverse_bind_matrices = NULL;
    }
      
    if ( gltf_skin->joints_count )
    {
      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( skin->joints, gltf_skin->joints_count, crude_heap_allocator_pack( manager->allocator ) );
      for ( uint32 i = 0; i < gltf_skin->joints_count; ++i )
      {
        skin->joints[ i ] = cgltf_node_index( gltf, gltf_skin->joints[ i ] );
      }
    }
    else
    {
      skin->joints = NULL;
    }
  }
}

void
crude_gfx_model_renderer_resources_manager_load_animations_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources,
  _In_ cgltf_data                                         *gltf
)
{
  CRUDE_HASHMAPSTR_INITIALIZE_WITH_CAPACITY( model_renderer_resources->animation_name_to_index, 4 * gltf->animations_count, crude_heap_allocator_pack( manager->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( model_renderer_resources->animations, gltf->animations_count, crude_heap_allocator_pack( manager->allocator ) );
  for ( uint32 animation_index = 0; animation_index < gltf->animations_count; ++animation_index )
  {
    cgltf_animation                                       *gltf_animation;
    crude_gfx_animation                                   *animation;
    
    gltf_animation = &gltf->animations[ animation_index ];
    animation = &model_renderer_resources->animations[ animation_index ];
    if ( gltf_animation->name[ 0 ] )
    {
      crude_string_copy( animation->name, gltf_animation->name, sizeof( animation->name ) );
    }
    else
    {
      crude_snprintf( animation->name, sizeof( animation->name ), "%s_unknown_animation_%i", model_renderer_resources->relative_filepath, animation_index );
    }

    CRUDE_HASHMAPSTR_SET( model_renderer_resources->animation_name_to_index, CRUDE_COMPOUNT( crude_string_link, { animation->name } ), animation_index );

    animation->start = 0.f;
    animation->end = 0.f;

    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( animation->samplers, gltf_animation->samplers_count, crude_heap_allocator_pack( manager->allocator ) );

    for ( uint64 sampler_index = 0; sampler_index < gltf_animation->samplers_count; ++sampler_index )
    {
      cgltf_animation_sampler                               *gltf_sampler;
      crude_gfx_animation_sampler                           *sampler;
      uint8                                                 *inputs_data;
      uint8                                                 *outputs_data;

      gltf_sampler = &gltf_animation->samplers[ sampler_index ];
      sampler = &animation->samplers[ sampler_index ];
    
      if ( gltf_sampler->interpolation == cgltf_interpolation_type_linear )
      {
        sampler->interpolation = CRUDE_GFX_ANIMATION_SAMPLER_INTERPOLATION_TYPE_LINEAR; 
      }
      else if ( gltf_sampler->interpolation == cgltf_interpolation_type_step )
      {
        sampler->interpolation = CRUDE_GFX_ANIMATION_SAMPLER_INTERPOLATION_TYPE_STEP;
      }
      else
      {
        CRUDE_ASSERT( false );
      }
    
      inputs_data = CRUDE_CAST( uint8*, gltf_sampler->input->buffer_view->buffer->data ) + gltf_sampler->input->buffer_view->offset;
      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( sampler->inputs, gltf_sampler->input->count, crude_heap_allocator_pack( manager->allocator ) );

      for ( uint64 i = 0; i < gltf_sampler->input->count; ++i )
      {
        sampler->inputs[ i ] = *CRUDE_CAST( float32*, inputs_data );
        inputs_data += gltf_sampler->input->stride;
      }
    
      for ( uint64 i = 0; i < gltf_sampler->input->count; ++i )
      {
        if ( sampler->inputs[ i ] < animation->start )
        {
          animation->start = sampler->inputs[ i ];
        }
        if ( sampler->inputs[ i ] > animation->end )
        {
          animation->end = sampler->inputs[ i ];
        }
      }
    
      CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( sampler->outputs, gltf_sampler->output->count, crude_heap_allocator_pack( manager->allocator ) );
      outputs_data = CRUDE_CAST( uint8*, gltf_sampler->output->buffer_view->buffer->data ) + gltf_sampler->output->buffer_view->offset + gltf_sampler->output->offset;
      
      for ( uint32 i = 0; i < gltf_sampler->output->count; ++i )
      {
        if ( gltf_sampler->output->type == cgltf_type_scalar )
        {
          float32 *data = CRUDE_CAST( float32*, outputs_data );
          sampler->outputs[ i ].x = *data;
          sampler->outputs[ i ].y = 0;
          sampler->outputs[ i ].z = 0;
          sampler->outputs[ i ].w = 0;
        }
        else if ( gltf_sampler->output->type == cgltf_type_vec3 )
        {
          XMFLOAT3 *data = CRUDE_CAST( XMFLOAT3*, outputs_data );
          sampler->outputs[ i ].x = data->x;
          sampler->outputs[ i ].y = data->y;
          sampler->outputs[ i ].z = data->z;
          sampler->outputs[ i ].w = 0;
        }
        else if ( gltf_sampler->output->type == cgltf_type_vec4 )
        {
          sampler->outputs[ i ] = *CRUDE_CAST( XMFLOAT4*, outputs_data );
        }
        else
        {
          CRUDE_ASSERT( false );
        }
        outputs_data += gltf_sampler->output->stride;
      }
    }

    CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( animation->channels, gltf_animation->channels_count, crude_heap_allocator_pack( manager->allocator ) );

    for ( uint64 i = 0u; i < gltf_animation->channels_count; ++i )
    {
      switch ( gltf_animation->channels[ i ].target_path )
      {
      case cgltf_animation_path_type_rotation:
      {
        animation->channels[ i ].path = CRUDE_GFX_ANIMATION_CHANNEL_PATH_ROTATION;
        break;
      }
      case cgltf_animation_path_type_scale:
      {
        animation->channels[ i ].path = CRUDE_GFX_ANIMATION_CHANNEL_PATH_SCALE;
        break;
      }
      case cgltf_animation_path_type_translation:
      {
        animation->channels[ i ].path = CRUDE_GFX_ANIMATION_CHANNEL_PATH_TRANSLATION;
        break;
      }
      }
      
      animation->channels[ i ].sampler_index = gltf_animation->channels[ i ].sampler - gltf_animation->samplers;
      
      animation->channels[ i ].node = cgltf_node_index( gltf, gltf_animation->channels[ i ].target_node );
    }
  }
}

#if CRUDE_GFX_RAY_TRACING_ENABLED
static void
crude_gfx_model_renderer_resources_manager_create_bottom_level_acceleration_structure_
(
  _In_ crude_gfx_model_renderer_resources_manager         *manager,
  _In_ crude_gfx_model_renderer_resources                 *model_renderer_resources
)
{
  VkAccelerationStructureGeometryKHR                      *vk_acceleration_structure_geometries;
  VkAccelerationStructureBuildRangeInfoKHR               **vk_acceleration_structure_build_range_infos;
  VkAccelerationStructureBuildGeometryInfoKHR             *vk_acceleration_build_geometry_infos;
  crude_gfx_memory_allocation                             *blas_scratch_buffers_hga;
  crude_gfx_cmd_buffer                                    *cmd_instant;
  uint32                                                   temporary_allocator_marker;
 
  model_renderer_resources->rtx_affected = ( CRUDE_ARRAY_LENGTH( model_renderer_resources->animations ) == 0 );

  if ( !model_renderer_resources->rtx_affected )
  {
    return;
  }

  temporary_allocator_marker = crude_stack_allocator_get_marker( manager->temporary_allocator );
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_acceleration_structure_geometries, CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ), crude_stack_allocator_pack( manager->temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_acceleration_structure_build_range_infos, CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ), crude_stack_allocator_pack( manager->temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( vk_acceleration_build_geometry_infos, CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ), crude_stack_allocator_pack( manager->temporary_allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( blas_scratch_buffers_hga, CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ), crude_stack_allocator_pack( manager->temporary_allocator ) );
  
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( model_renderer_resources->blases_hga, CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ), crude_heap_allocator_pack( manager->allocator ) );
  CRUDE_ARRAY_INITIALIZE_WITH_LENGTH( model_renderer_resources->vk_blases, CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ), crude_heap_allocator_pack( manager->allocator ) );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( model_renderer_resources->meshes ); ++i )
  {
    crude_gfx_mesh_cpu const                              *mesh;
    VkAccelerationStructureBuildSizesInfoKHR               vk_acceleration_structure_build_sizes_info;
    VkAccelerationStructureCreateInfoKHR                   vk_acceleration_structure_create_info;
    uint32                                                 max_primitives_count;
    
    mesh = &model_renderer_resources->meshes[ i ];
    
    max_primitives_count = mesh->indices_count / 3;
  
    vk_acceleration_structure_geometries[ i ] = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureGeometryKHR );
    vk_acceleration_structure_geometries[ i ].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    vk_acceleration_structure_geometries[ i ].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    vk_acceleration_structure_geometries[ i ].flags = ( mesh->flags & CRUDE_MESH_DRAW_FLAGS_TRANSLUCENT_MASK ) ? 0 : VK_GEOMETRY_OPAQUE_BIT_KHR;
    vk_acceleration_structure_geometries[ i ].geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    vk_acceleration_structure_geometries[ i ].geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    vk_acceleration_structure_geometries[ i ].geometry.triangles.vertexData.deviceAddress = manager->meshlets_vertices_positions_hga.gpu_address;
    vk_acceleration_structure_geometries[ i ].geometry.triangles.vertexStride = sizeof( crude_gfx_vertex_position );
    vk_acceleration_structure_geometries[ i ].geometry.triangles.maxVertex = max_primitives_count;
    vk_acceleration_structure_geometries[ i ].geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    vk_acceleration_structure_geometries[ i ].geometry.triangles.indexData.deviceAddress = mesh->index_hga.gpu_address;
    vk_acceleration_structure_geometries[ i ].geometry.triangles.transformData.deviceAddress = manager->bottom_level_acceleration_structure_transform_hga.gpu_address;
    
    vk_acceleration_structure_build_range_infos[ i ] = CRUDE_CAST( VkAccelerationStructureBuildRangeInfoKHR*, crude_stack_allocator_allocate( manager->temporary_allocator, sizeof( VkAccelerationStructureBuildRangeInfoKHR ) ) );
    *vk_acceleration_structure_build_range_infos[ i ] = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildRangeInfoKHR );
    vk_acceleration_structure_build_range_infos[ i ]->primitiveCount = max_primitives_count;
    vk_acceleration_structure_build_range_infos[ i ]->primitiveOffset = 0;
    vk_acceleration_structure_build_range_infos[ i ]->transformOffset = 0;
    
    vk_acceleration_build_geometry_infos[ i ] = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildGeometryInfoKHR );
    vk_acceleration_build_geometry_infos[ i ].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    vk_acceleration_build_geometry_infos[ i ].type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    vk_acceleration_build_geometry_infos[ i ].flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    vk_acceleration_build_geometry_infos[ i ].mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    vk_acceleration_build_geometry_infos[ i ].geometryCount = 1u;
    vk_acceleration_build_geometry_infos[ i ].pGeometries = &vk_acceleration_structure_geometries[ i ];
    
    vk_acceleration_structure_build_sizes_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureBuildSizesInfoKHR );
    vk_acceleration_structure_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    manager->gpu->vkGetAccelerationStructureBuildSizesKHR( manager->gpu->vk_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &vk_acceleration_build_geometry_infos[ i ], &max_primitives_count, &vk_acceleration_structure_build_sizes_info );
  
    model_renderer_resources->blases_hga[ i ] = crude_gfx_memory_allocate_with_name( manager->gpu, vk_acceleration_structure_build_sizes_info.accelerationStructureSize, CRUDE_GFX_MEMORY_TYPE_GPU, "blas_hga", VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR );
  
    vk_acceleration_structure_create_info = CRUDE_COMPOUNT_EMPTY( VkAccelerationStructureCreateInfoKHR );
    vk_acceleration_structure_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    vk_acceleration_structure_create_info.buffer = crude_gfx_access_buffer( manager->gpu, model_renderer_resources->blases_hga[ i ].buffer_handle )->vk_buffer;
    vk_acceleration_structure_create_info.offset = 0;
    vk_acceleration_structure_create_info.size = vk_acceleration_structure_build_sizes_info.accelerationStructureSize;
    vk_acceleration_structure_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    CRUDE_GFX_HANDLE_VULKAN_RESULT( manager->gpu->vkCreateAccelerationStructureKHR( manager->gpu->vk_device, &vk_acceleration_structure_create_info, manager->gpu->vk_allocation_callbacks, &model_renderer_resources->vk_blases[ i ] ), "Can't create acceleration structure" );
    
    // TODO maybe we can use only one scratch buffer? idk for now
    blas_scratch_buffers_hga[ i ] = crude_gfx_memory_allocate_with_name( manager->gpu, vk_acceleration_structure_build_sizes_info.buildScratchSize, CRUDE_GFX_MEMORY_TYPE_GPU, "blas_scratch_hga", VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR );
  
    vk_acceleration_build_geometry_infos[ i ].dstAccelerationStructure = model_renderer_resources->vk_blases[ i ];
    vk_acceleration_build_geometry_infos[ i ].scratchData.deviceAddress = blas_scratch_buffers_hga[ i ].gpu_address;
  }
  
  cmd_instant = crude_gfx_access_cmd_buffer( manager->gpu, manager->gpu->immediate_transfer_cmd_buffer );
  crude_gfx_cmd_begin_primary( cmd_instant );
  manager->gpu->vkCmdBuildAccelerationStructuresKHR( cmd_instant->vk_cmd_buffer, CRUDE_ARRAY_LENGTH( vk_acceleration_build_geometry_infos ), vk_acceleration_build_geometry_infos, vk_acceleration_structure_build_range_infos );
  crude_gfx_submit_immediate( cmd_instant );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( blas_scratch_buffers_hga ); ++i )
  {
    crude_gfx_memory_deallocate( manager->gpu, blas_scratch_buffers_hga[ i ] );
  }
  
  crude_stack_allocator_free_marker( manager->temporary_allocator, temporary_allocator_marker );
}
#endif /* CRUDE_GFX_RAY_TRACING_ENABLED */