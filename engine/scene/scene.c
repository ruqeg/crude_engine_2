#include <cgltf.h>
#include <stb_image.h>

#include <core/algorithms.h>
#include <core/file.h>

#include <scene/scene.h>

// !TODO
#include <crude_shaders/main.frag.inl>
#include <crude_shaders/main.vert.inl>

#define _PARALLEL_RECORDINGS 4

/**
 *
 * GLTF Draw Task
 * 
 */
typedef struct _gltf_scene_primary_draw_task_data
{
  enkiTaskScheduler*                                       task_scheduler;
  crude_gltf_scene                                        *scene;
  uint32                                                   thread_id;
  bool                                                     use_secondary;
} _gltf_scene_primary_draw_task_data;

typedef struct _gltf_scene_secondary_draw_task_data
{
  crude_gltf_scene                                        *scene;
  crude_gfx_cmd_buffer                                    *parent_cmd;
  uint32                                                   thread_id;
  uint32                                                   start_mesh_draw_index;
  uint32                                                   end_mesh_draw_index;
  crude_gfx_cmd_buffer                                    *secondary_cmd;
} _gltf_scene_secondary_draw_task_data;

void
_gltf_scene_primary_draw_task
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *args
);

void
_gltf_scene_secondary_draw_task
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *args
);

/**
 *
 * GLTF Utils Functinos Declaration
 * 
 */
static void
_get_gltf_mesh_vertex_buffer
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gltf_scene                                   *scene,
  _In_ int32                                               accessor_index,
  _Out_ crude_gfx_buffer_handle                           *buffer_handle,
  _Out_ uint32                                            *buffer_offset
);

static bool
_create_gltf_mesh_material
(
  _In_ cgltf_data                               *gltf,
  _In_ crude_gfx_renderer                       *renderer,
  _In_ crude_gltf_scene                         *scene,
  _In_ cgltf_material                           *material,
  _In_ crude_mesh_draw                          *mesh_draw
);

static void
_draw_mesh
(
  _In_ crude_gfx_cmd_buffer                               *gpu_commands,
  _In_ crude_mesh_draw                                    *mesh_draw
);

/**
 *
 * GLTF Scene Functinos
 * 
 */
void
crude_gltf_scene_load_from_file
(
  _In_ crude_gltf_scene                                   *scene,
  _In_ crude_gltf_scene_creation const                    *creation
)
{
  scene->async_loader = creation->async_loader;
  scene->renderer = creation->renderer;
  CRUDE_ASSERT( strcpy_s( scene->path, sizeof( scene->path ), creation->path ) == 0 );

  cgltf_options gltf_options = { 0 };
  cgltf_data *gltf = NULL;
  cgltf_result result = cgltf_parse_file( &gltf_options, scene->path, &gltf );
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

  crude_gfx_pipeline_creation pipeline_creation;
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
  
  pipeline_creation.depth_stencil.depth_write_enable = true;
  pipeline_creation.depth_stencil.depth_enable = true;
  pipeline_creation.depth_stencil.depth_comparison = VK_COMPARE_OP_LESS;

  crude_gfx_renderer_program_creation program_creation = { 
    .pipeline_creation = pipeline_creation
  };
  scene->program = crude_gfx_renderer_create_program( scene->renderer, &program_creation );
  
  crude_gfx_renderer_material_creation material_creatoin = { 
    .name = "material1",
    .program = scene->program,
    .render_index = 0,
  };
  scene->material = crude_gfx_renderer_create_material( scene->renderer, &material_creatoin );

  char prev_directory[ 1024 ];
  crude_get_current_working_directory( &prev_directory, sizeof( prev_directory ) );
  
  char gltf_base_path[ 1024 ];
  memcpy( gltf_base_path, scene->path, sizeof( gltf_base_path ) );
  crude_file_directory_from_path( gltf_base_path );

  scene->images = NULL;
  CRUDE_ARR_SETCAP( scene->images, gltf->images_count );
  
  for ( uint32 image_index = 0; image_index < gltf->images_count; ++image_index )
  {
    cgltf_image const *image = &gltf->images[ image_index ];

    char image_full_filename[ 512 ] = { 0 };
    strcat( image_full_filename, gltf_base_path );
    strcat( image_full_filename, image->uri );

    int comp, width, height;
    stbi_info( image_full_filename, &width, &height, &comp );

    crude_gfx_texture_creation texture_creation = {
      .initial_data = NULL,
      .width = width,
      .height = height,
      .depth = 1u,
      .mipmaps = 1u,
      .flags = 0u,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .type = CRUDE_GFX_TEXTURE_TYPE_TEXTURE_2D,
      .name = image_full_filename,
    };

    crude_gfx_renderer_texture *texture_resource = crude_gfx_renderer_create_texture( scene->renderer, &texture_creation );
    CRUDE_ARR_PUSH( scene->images, *texture_resource );

    crude_gfx_asynchronous_loader_request_texture_data( scene->async_loader, image_full_filename, texture_resource->handle );
  }

  // Load all samplers
  scene->samplers = NULL;
  CRUDE_ARR_SETCAP( scene->samplers, gltf->samplers_count );

  for ( uint32 sampler_index = 0; sampler_index < gltf->samplers_count; ++sampler_index )
  {
    cgltf_sampler *sampler = &gltf->samplers[ sampler_index ];
  
    crude_gfx_sampler_creation creation;
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
    
    crude_gfx_renderer_sampler *sampler_resource = crude_gfx_renderer_create_sampler( scene->renderer, &creation );
    CRUDE_ARR_PUSH( scene->samplers, *sampler_resource );
  }
  
  scene->buffers = NULL;
  CRUDE_ARR_SETCAP( scene->buffers, gltf->buffer_views_count );

  for ( uint32 buffer_view_index = 0; buffer_view_index < gltf->buffer_views_count; ++buffer_view_index )
  {
    cgltf_buffer_view *buffer_view = &gltf->buffer_views[ buffer_view_index ];
    cgltf_buffer *buffer = buffer_view->buffer;
  
    uint8* data = ( uint8* )buffer->data + buffer_view->offset;
  
    if ( buffer_view->name == NULL )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_FILEIO, "Bufer name is null: %u", buffer_view_index );
    }
    
    crude_gfx_buffer_creation cpu_buffer_creation = {
      .initial_data = data,
      .usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE,
      .size = buffer_view->size,
      .type_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .name = buffer->name
    };
    crude_gfx_buffer_handle cpu_buffer = crude_gfx_create_buffer( scene->renderer->gpu, &cpu_buffer_creation );

    crude_gfx_buffer_creation gpu_buffer_creation = {
      .usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_IMMUTABLE,
      .size = buffer_view->size,
      .type_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .name = buffer->name,
      .device_only = true
    };
    crude_gfx_renderer_buffer *gpu_buffer_resource = crude_gfx_renderer_create_buffer( scene->renderer, &gpu_buffer_creation );
    CRUDE_ARR_PUSH( scene->buffers, *gpu_buffer_resource );

    crude_gfx_asynchronous_loader_request_buffer_copy( scene->async_loader, cpu_buffer, gpu_buffer_resource->handle );
  }

  scene->mesh_draws = NULL;
  CRUDE_ARR_SETCAP( scene->mesh_draws, gltf->meshes_count );

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
      crude_mesh_draw mesh_draw = { .scale = node_scale, .translation = node_translation, .rotation = node_rotation };
      
      cgltf_primitive *mesh_primitive = &node->mesh->primitives[ primitive_index ];

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
      
      cgltf_accessor *indices_accessor = mesh_primitive->indices;
      cgltf_buffer_view *indices_buffer_view = indices_accessor->buffer_view;
      
      crude_gfx_renderer_buffer *indices_buffer_gpu = &scene->buffers[ cgltf_buffer_view_index( gltf, indices_accessor->buffer_view ) ];
      mesh_draw.index_buffer = indices_buffer_gpu->handle;
      mesh_draw.index_offset = indices_accessor->offset;
      mesh_draw.primitive_count = indices_accessor->count;

      bool transparent = _create_gltf_mesh_material( gltf, scene->renderer, scene, mesh_primitive->material, &mesh_draw );
      
      //!TODO
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
      CRUDE_ARR_PUSH( scene->mesh_draws, mesh_draw );
    }
  }
  cgltf_free( gltf );
}

void
crude_gltf_scene_unload
(
  _In_ crude_gltf_scene                                   *scene
)
{
  crude_gfx_renderer_destroy_program( scene->renderer, scene->program );
  crude_gfx_renderer_destroy_material( scene->renderer, scene->material );

  for ( uint32 i = 0; i < CRUDE_ARR_LEN( scene->images ); ++i )
  {
    crude_gfx_renderer_destroy_texture( scene->renderer, &scene->images[ i ] );
  }
  CRUDE_ARR_FREE( scene->images );

  for ( uint32 i = 0; i < CRUDE_ARR_LEN( scene->samplers ); ++i )
  {
    crude_gfx_renderer_destroy_sampler( scene->renderer, &scene->samplers[ i ] );
  }
  CRUDE_ARR_FREE( scene->samplers );
  
  for ( uint32 i = 0; i < CRUDE_ARR_LEN( scene->buffers ); ++i )
  {
    crude_gfx_renderer_destroy_buffer( scene->renderer, &scene->buffers[ i ] );
  }
  CRUDE_ARR_FREE( scene->buffers );

  for ( uint32 i = 0; i < CRUDE_ARR_LEN( scene->mesh_draws ); ++i )
  {
     crude_gfx_destroy_buffer( scene->renderer->gpu, scene->mesh_draws[ i ].material_buffer );
  }
  CRUDE_ARR_FREE( scene->mesh_draws );

  CRUDE_ARR_FREE( scene->buffers );
}

void
crude_gltf_scene_submit_draw_task
(
  _In_ crude_gltf_scene                                   *scene,
  _In_ enkiTaskScheduler                                  *task_sheduler,
  _In_ bool                                                use_secondary
)
{
  _gltf_scene_primary_draw_task_data draw_task_data = { 
    .scene = scene,
    .use_secondary = use_secondary,
    .task_scheduler = task_sheduler
  };
  enkiTaskSet *draw_task = enkiCreateTaskSet( task_sheduler, _gltf_scene_primary_draw_task );
  enkiSetArgsTaskSet( draw_task, &draw_task_data );
  enkiAddTaskSet( task_sheduler, draw_task );
  enkiWaitForTaskSet( task_sheduler, draw_task );
  crude_gfx_renderer_add_texture_update_commands( scene->renderer, ( draw_task_data.thread_id + 1 ) % enkiGetNumTaskThreads( task_sheduler ) );
}

/**
 *
 * GLTF Utils Functinos Implementation
 * 
 */
void
_get_gltf_mesh_vertex_buffer
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gltf_scene                                   *scene,
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
  _In_ cgltf_data                               *gltf,
  _In_ crude_gfx_renderer                       *renderer,
  _In_ crude_gltf_scene                         *scene,
  _In_ cgltf_material                           *material,
  _In_ crude_mesh_draw                          *mesh_draw
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
    mesh_draw->albedo_texture_index = CRUDE_GFX_RENDERER_INVALID_TEXTURE_INDEX;
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
    mesh_draw->roughness_texture_index = CRUDE_GFX_RENDERER_INVALID_TEXTURE_INDEX;
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
    mesh_draw->occlusion_texture_index = CRUDE_GFX_RENDERER_INVALID_TEXTURE_INDEX;
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
    mesh_draw->normal_texture_index = CRUDE_GFX_RENDERER_INVALID_TEXTURE_INDEX;
  }
  
  crude_gfx_buffer_creation buffer_creation = {
    .usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC,
    .type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    .size = sizeof ( crude_gfx_shader_mesh_constants ),
    .name = "mesh_data"
  };
  mesh_draw->material_buffer = crude_gfx_create_buffer( renderer->gpu, &buffer_creation );
  
  return transparent;
}

void
_draw_mesh
(
  _In_ crude_gfx_cmd_buffer                               *gpu_commands,
  _In_ crude_mesh_draw                                    *mesh_draw
)
{
  bool mesh_buffers_ready = crude_gfx_buffer_ready( gpu_commands->gpu, mesh_draw->position_buffer )
    && crude_gfx_buffer_ready( gpu_commands->gpu, mesh_draw->tangent_buffer )
    && crude_gfx_buffer_ready( gpu_commands->gpu, mesh_draw->normal_buffer )
    && crude_gfx_buffer_ready( gpu_commands->gpu, mesh_draw->texcoord_buffer )
    && crude_gfx_buffer_ready( gpu_commands->gpu, mesh_draw->index_buffer );

  if ( !mesh_buffers_ready )
  {
    return;
  }

  crude_gfx_descriptor_set_creation ds_creation = {
    .samplers = { CRUDE_GFX_INVALID_SAMPLER_HANDLE, CRUDE_GFX_INVALID_SAMPLER_HANDLE },
    .bindings = { 0, 1 },
    .resources= { gpu_commands->gpu->frame_buffer.index, mesh_draw->material_buffer.index },
    .num_resources = 2,
    .layout = mesh_draw->material->program->passes[ 0 ].descriptor_set_layout
  };
  crude_gfx_descriptor_set_handle descriptor_set = crude_gfx_cmd_create_local_descriptor_set( gpu_commands, &ds_creation );

  crude_gfx_cmd_bind_vertex_buffer( gpu_commands, mesh_draw->position_buffer, 0, mesh_draw->position_offset );
  crude_gfx_cmd_bind_vertex_buffer( gpu_commands, mesh_draw->tangent_buffer, 1, mesh_draw->tangent_offset );
  crude_gfx_cmd_bind_vertex_buffer( gpu_commands, mesh_draw->normal_buffer, 2, mesh_draw->normal_offset );
  crude_gfx_cmd_bind_vertex_buffer( gpu_commands, mesh_draw->texcoord_buffer, 3, mesh_draw->texcoord_offset );
  crude_gfx_cmd_bind_index_buffer( gpu_commands, mesh_draw->index_buffer, mesh_draw->index_offset );
  crude_gfx_cmd_bind_local_descriptor_set( gpu_commands, descriptor_set );
  crude_gfx_cmd_draw_indexed( gpu_commands, mesh_draw->primitive_count, 1, 0, 0, 0 );
}

/**
 *
 * GLTF Draw Task
 * 
 */
void
_gltf_scene_primary_draw_task
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *args
)
{
  _gltf_scene_primary_draw_task_data *draw_task = CAST( _gltf_scene_primary_draw_task_data *, args );
  
  draw_task->thread_id = thread_num;

  crude_gfx_cmd_buffer *gpu_commands = crude_gfx_get_primary_cmd( draw_task->scene->renderer->gpu, draw_task->thread_id, true );

  crude_gfx_cmd_set_clear_color( gpu_commands, 0, ( VkClearValue ) { .color = { 0, 0, 0, 0 } });
  crude_gfx_cmd_set_clear_color( gpu_commands, 1, ( VkClearValue ) { .color = { 1, 1, 1, 1 } });
  crude_gfx_cmd_bind_render_pass( gpu_commands, draw_task->scene->renderer->gpu->swapchain_pass, draw_task->use_secondary );
  
  if ( draw_task->use_secondary )
  {
    uint32 draws_per_secondary = CRUDE_ARR_LEN( draw_task->scene->mesh_draws ) / _PARALLEL_RECORDINGS;
    uint32 offset = draws_per_secondary * _PARALLEL_RECORDINGS;
    
    enkiTaskSet *secondary_draw_tasks[ _PARALLEL_RECORDINGS ];
    _gltf_scene_secondary_draw_task_data secondary_draw_tasks_data[ _PARALLEL_RECORDINGS ];
    _gltf_scene_secondary_draw_task_data offset_secondary_draw_tasks_data;
    
    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      secondary_draw_tasks_data[ i ] = ( _gltf_scene_secondary_draw_task_data ) {
        .scene = draw_task->scene,
        .parent_cmd = gpu_commands,
        .start_mesh_draw_index = draws_per_secondary * i,
        .end_mesh_draw_index = draws_per_secondary * i + draws_per_secondary
      };
      
      secondary_draw_tasks[ i ] = enkiCreateTaskSet( draw_task->task_scheduler, _gltf_scene_secondary_draw_task );
      enkiSetArgsTaskSet( secondary_draw_tasks[ i ], &secondary_draw_tasks_data[ i ] );
      enkiAddTaskSet( draw_task->task_scheduler, secondary_draw_tasks[ i ] );
    }
    
    if ( offset < CRUDE_ARR_LEN( draw_task->scene->mesh_draws ) )
    {
      offset_secondary_draw_tasks_data = ( _gltf_scene_secondary_draw_task_data ) {
        .scene = draw_task->scene,
        .parent_cmd = gpu_commands,
        .start_mesh_draw_index = offset,
        .end_mesh_draw_index = CRUDE_ARR_LEN( draw_task->scene->mesh_draws )
      };
      _gltf_scene_secondary_draw_task( NULL, NULL, thread_num, &offset_secondary_draw_tasks_data );
    }
    
    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      enkiWaitForTaskSet( draw_task->task_scheduler, secondary_draw_tasks[ i ] );
      vkCmdExecuteCommands( gpu_commands->vk_cmd_buffer, 1, &secondary_draw_tasks_data[ i ].secondary_cmd->vk_cmd_buffer );
    }
    
    if ( offset < CRUDE_ARR_LEN( draw_task->scene->mesh_draws ) )
    {
      vkCmdExecuteCommands( gpu_commands->vk_cmd_buffer, 1, &offset_secondary_draw_tasks_data.secondary_cmd->vk_cmd_buffer );
    }
    crude_gfx_cmd_end_render_pass( gpu_commands );
  }
  else
  {
    crude_gfx_cmd_set_viewport( gpu_commands, NULL );
    crude_gfx_cmd_set_scissor( gpu_commands, NULL );

    crude_gfx_renderer_material *last_material = NULL;
    for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARR_LEN( draw_task->scene->mesh_draws ); ++mesh_index )
    {
      crude_mesh_draw *mesh_draw = &draw_task->scene->mesh_draws[ mesh_index ];
      if ( mesh_draw->material != last_material )
      {
        crude_gfx_cmd_bind_pipeline( gpu_commands, mesh_draw->material->program->passes[ 0 ].pipeline );
        last_material = mesh_draw->material;
      }
      _draw_mesh( gpu_commands, mesh_draw );
    }
  }
  crude_gfx_queue_cmd( gpu_commands );
}

void
_gltf_scene_secondary_draw_task
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *args
)
{
  _gltf_scene_secondary_draw_task_data *secondary_draw_task = CAST( _gltf_scene_secondary_draw_task_data *, args );
  
  crude_gfx_cmd_buffer *secondary_cmd = crude_gfx_get_secondary_cmd( secondary_draw_task->scene->renderer->gpu, thread_num );
  
  crude_gfx_cmd_begin_secondary( secondary_cmd, secondary_draw_task->parent_cmd->current_render_pass );
  crude_gfx_cmd_set_viewport( secondary_cmd, NULL );
  crude_gfx_cmd_set_scissor( secondary_cmd, NULL );
  
  crude_gfx_renderer_material *last_material = NULL;
  for ( uint32 mesh_index = secondary_draw_task->start_mesh_draw_index; mesh_index < secondary_draw_task->end_mesh_draw_index; ++mesh_index )
  {
    crude_mesh_draw const *mesh_draw = &secondary_draw_task->scene->mesh_draws[ mesh_index ];
  
    if ( mesh_draw->material != last_material )
    {
      crude_gfx_cmd_bind_pipeline( secondary_cmd, mesh_draw->material->program->passes[ 0 ].pipeline );
      last_material = mesh_draw->material;
    }
    
    _draw_mesh( secondary_cmd, mesh_draw );
  }
  
  crude_gfx_cmd_end( secondary_cmd );

  secondary_draw_task->secondary_cmd = secondary_cmd;
}