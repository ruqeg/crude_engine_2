#include <TaskScheduler_c.h>
#include <cgltf.h>
#include <stb_image.h>

#include <core/ecs_utils.h>
#include <core/profiler.h>
#include <core/array.h>
#include <core/file.h>
#include <core/hash_map.h>
#include <scene/scene_components.h>

#include <graphics/scene_renderer.h>

/**
 *
 * Scene Draw Task Constants
 * 
 */
#define _PARALLEL_RECORDINGS                               ( 4 )

/************************************************
 *
 * GLTF Utils Functinos Declaration
 * 
 ***********************************************/
static bool
create_gltf_mesh_material_
(
  _In_ cgltf_data                                         *gltf,
  _In_ crude_gfx_renderer                                 *renderer,
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ cgltf_material                                     *material,
  _In_ crude_gfx_mesh                                     *mesh_draw,
  _In_ size_t                                             scene_renderer_images_offset,
  _In_ size_t                                             scene_renderer_samplers_offset
);

static void
upload_gltf_to_scene_renderer_
(
  _In_ crude_gfx_scene_renderer                          *scene_renderer,
  _In_ char const                                        *gltf_path,
  _In_ char const                                        *resoruces_dir,
  _In_ crude_stack_allocator                             *temporary_allocator
);

/**
 *
 * Renderer Scene Draw Task Declaration
 * 
 */
typedef struct secondary_draw_task_container
{
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass;
  crude_gfx_cmd_buffer                                    *primary_cmd;
  crude_gfx_cmd_buffer                                    *secondary_cmd;
  uint32                                                   start_mesh_draw_index;
  uint32                                                   end_mesh_draw_index;
  uint32                                                   thread_id;
} secondary_draw_task_container;

static void
secondary_draw_task_
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *ctx
);

/**
 *
 * Renderer Scene Draw Declaration
 * 
 */
static void
draw_mesh_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_mesh                                     *mesh
);

static void
draw_scene_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass
);

/**
 *
 * Common Renderer Scene Structes
 * 
 */
bool
crude_gfx_mesh_is_transparent
(
  _In_ crude_gfx_mesh                                     *mesh
)
{
  return ( mesh->flags & ( CRUDE_GFX_DRAW_FLAGS_ALPHA_MASK | CRUDE_GFX_DRAW_FLAGS_TRANSPARENT_MASK ) ) != 0;
}

/**
 *
 * Renderer Scene Geometry Pass
 * 
 */
static void
crude_gfx_render_graph_pass_container_pre_render_empry
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
}

static void
crude_gfx_render_graph_pass_container_on_resize_empty
(
  _In_ void                                               *ctx,
  _In_ crude_gfx_device                                   *gpu,
  _In_ uint32                                              new_width,
  _In_ uint32                                              new_height
)
{
}

void
crude_gfx_scene_renderer_geometry_pass_render
(
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass,
  _In_ crude_gfx_cmd_buffer                               *primary_cmd
)
{
  bool use_secondary = true;
  if ( use_secondary )
  {
    enkiTaskSet                                           *secondary_draw_tasks[ _PARALLEL_RECORDINGS ];
    secondary_draw_task_container                          secondary_draw_tasks_data[ _PARALLEL_RECORDINGS ];
    secondary_draw_task_container                          offset_secondary_draw_tasks_data;
    uint32                                                 draws_per_secondary, offset;

    draws_per_secondary = CRUDE_ARRAY_LENGTH( pass->mesh_instances ) / _PARALLEL_RECORDINGS;
    offset = draws_per_secondary * _PARALLEL_RECORDINGS;

    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      secondary_draw_tasks_data[ i ] = ( secondary_draw_task_container ) {
        .pass                   = pass,
        .primary_cmd            = primary_cmd,
        .start_mesh_draw_index  = draws_per_secondary * i,
        .end_mesh_draw_index    = draws_per_secondary * i + draws_per_secondary
      };
      
      secondary_draw_tasks[ i ] = enkiCreateTaskSet( pass->scene->task_scheduler, secondary_draw_task_ );
      enkiSetArgsTaskSet( secondary_draw_tasks[ i ], &secondary_draw_tasks_data[ i ] );
      enkiAddTaskSet( pass->scene->task_scheduler, secondary_draw_tasks[ i ] );
    }
    
    if ( offset < CRUDE_ARRAY_LENGTH( pass->mesh_instances ) )
    {
      offset_secondary_draw_tasks_data = ( secondary_draw_task_container ) {
        .pass                   = pass,
        .primary_cmd            = primary_cmd,
        .start_mesh_draw_index  = offset,
        .end_mesh_draw_index    = CRUDE_ARRAY_LENGTH( pass->mesh_instances )
      };
      secondary_draw_task_( NULL, NULL, 0, &offset_secondary_draw_tasks_data );
    }
    
    for ( uint32 i = 0; i < _PARALLEL_RECORDINGS; ++i )
    {
      enkiWaitForTaskSet( pass->scene->task_scheduler, secondary_draw_tasks[ i ] );
      vkCmdExecuteCommands( primary_cmd->vk_cmd_buffer, 1, &secondary_draw_tasks_data[ i ].secondary_cmd->vk_cmd_buffer );
    }
    
    if ( offset < CRUDE_ARRAY_LENGTH( pass->mesh_instances ) )
    {
      vkCmdExecuteCommands( primary_cmd->vk_cmd_buffer, 1, &offset_secondary_draw_tasks_data.secondary_cmd->vk_cmd_buffer );
    }
  }
  else
  {
    draw_scene_( primary_cmd, pass );
  }
}

void
crude_gfx_scene_renderer_geometry_pass_prepare_draws
(
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass,
  _In_ crude_gfx_render_graph                             *render_graph,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  crude_gfx_render_graph_node                             *geometry_pass_node;
  crude_gfx_renderer_material                             *main_material;
  crude_gfx_renderer_technique                            *main_technique;
  uint64                                                   main_technique_name_hashed, main_technique_index;

  main_technique_name_hashed = crude_hash_bytes( ( void* )"main", strlen( "main" ), 0 );
  main_technique_index = CRUDE_HASHMAP_GET_INDEX( pass->scene->renderer->resource_cache.techniques, main_technique_name_hashed );
  if ( main_technique_index < 0)
  {
    CRUDE_ASSERT( false );
    return;
  }

  main_technique = pass->scene->renderer->resource_cache.techniques[ main_technique_index ].value;

  {
    crude_gfx_renderer_material_creation material_creation = {
      .name         = "material_no_cull",
      .technique    = main_technique,
      .render_index = 0
    };
    main_material = crude_gfx_renderer_create_material( pass->scene->renderer, &material_creation );
  }

  geometry_pass_node = crude_gfx_render_graph_builder_access_node_by_name( render_graph->builder, "geometry_pass" );
  CRUDE_ASSERT ( geometry_pass_node );
  

  CRUDE_ARRAY_INITIALIZE_WITH_CAPACITY( pass->mesh_instances, 16, pass->scene->renderer->allocator_container );
  for ( size_t i = 0; i < CRUDE_ARRAY_LENGTH( pass->scene->meshes ); ++i )
  {
    crude_gfx_mesh_instance                                mesh_instance;
    crude_gfx_mesh                                        *mesh;

    mesh = &pass->scene->meshes[ i ];
    if ( crude_gfx_mesh_is_transparent( mesh ) )
    {
      continue;
    }
  
    mesh->material = main_material;
    mesh_instance.mesh = mesh;
    mesh_instance.material_pass_index = 0;
    CRUDE_ARRAY_PUSH( pass->mesh_instances, mesh_instance );
  }
}

crude_gfx_render_graph_pass_container
crude_gfx_scene_renderer_geometry_pass_pack
(
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass
)
{
  crude_gfx_render_graph_pass_container container = {
    .pre_render = crude_gfx_render_graph_pass_container_pre_render_empry,
    .render     = crude_gfx_scene_renderer_geometry_pass_render,
    .on_resize  = crude_gfx_render_graph_pass_container_on_resize_empty,
    .ctx        = pass 
  };
  return container;
}

/**
 *
 * Renderer Scene Function
 * 
 */
void
crude_gfx_scene_renderer_initialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_scene_renderer_creation                  *creation
)
{
  scene_renderer->allocator_container = creation->allocator_container;
  scene_renderer->renderer = creation->renderer;
  scene_renderer->async_loader = creation->async_loader;
  scene_renderer->task_scheduler = creation->task_scheduler;

  scene_renderer->geometry_pass.scene = scene_renderer;
  scene_renderer->images = NULL;
  scene_renderer->samplers = NULL;
  scene_renderer->buffers = NULL;
  scene_renderer->meshes = NULL;
}

void
crude_gfx_scene_renderer_deinitialize
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer
)
{
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->images ); ++i )
  {
    crude_gfx_renderer_destroy_texture( scene_renderer->renderer, &scene_renderer->images[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->images );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->samplers ); ++i )
  {
    crude_gfx_renderer_destroy_sampler( scene_renderer->renderer, &scene_renderer->samplers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->samplers );
  
  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->buffers ); ++i )
  {
    crude_gfx_renderer_destroy_buffer( scene_renderer->renderer, &scene_renderer->buffers[ i ] );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->buffers );

  for ( uint32 i = 0; i < CRUDE_ARRAY_LENGTH( scene_renderer->meshes ); ++i )
  {
     crude_gfx_destroy_buffer( scene_renderer->renderer->gpu, scene_renderer->meshes[ i ].material_buffer );
  }
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->meshes );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->buffers );
  CRUDE_ARRAY_DEINITIALIZE( scene_renderer->geometry_pass.mesh_instances );
}

void
crude_gfx_scene_renderer_prepare_draws
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  crude_gfx_scene_renderer_geometry_pass_prepare_draws( &scene_renderer->geometry_pass, scene_renderer->render_graph, temporary_allocator );
}

void
crude_gfx_scene_renderer_submit_draw_task
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ bool                                                use_secondary
)
{
  crude_gfx_cmd_buffer *gpu_commands = crude_gfx_get_primary_cmd( scene_renderer->renderer->gpu, 0, true );
  crude_gfx_render_graph_render( scene_renderer->render_graph, gpu_commands );
  crude_gfx_queue_cmd( gpu_commands );
}

void
crude_gfx_scene_renderer_register_render_passes
(
  _In_ crude_gfx_scene_renderer                           *scene_renderer,
  _In_ crude_gfx_render_graph                             *render_graph
)
{
  scene_renderer->render_graph = render_graph;

  crude_gfx_render_graph_builder_register_render_pass( render_graph->builder, "geometry_pass", crude_gfx_scene_renderer_geometry_pass_pack( &scene_renderer->geometry_pass ) );
}

/**
 *
 * Renderer Scene Draw Task Implementation
 * 
 */
void
secondary_draw_task_
(
  _In_ uint32_t                                            start,
  _In_ uint32_t                                            end,
  _In_ uint32_t                                            thread_num,
  _In_ void                                               *ctx
)
{
  CRUDE_PROFILER_SET_THREAD_NAME( "SecondaryDrawTaskThread" );

  secondary_draw_task_container *secondary_draw_task = ctx;
  
  crude_gfx_cmd_buffer *secondary_cmd = crude_gfx_get_secondary_cmd( secondary_draw_task->pass->scene->renderer->gpu, thread_num );
  crude_gfx_cmd_begin_secondary( secondary_cmd, secondary_draw_task->primary_cmd->current_render_pass, secondary_draw_task->primary_cmd->current_framebuffer );
  
  crude_gfx_cmd_set_viewport( secondary_cmd, NULL );
  crude_gfx_cmd_set_scissor( secondary_cmd, NULL );
  draw_scene_( secondary_cmd, secondary_draw_task->pass );

  crude_gfx_cmd_end( secondary_cmd );
  secondary_draw_task->secondary_cmd = secondary_cmd;
}

/**
 *
 * Renderer Scene Draw Implementation
 * 
 */
void
draw_mesh_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_mesh                                     *mesh
)
{
  bool mesh_buffers_ready = crude_gfx_buffer_ready( cmd->gpu, mesh->position_buffer )
    && crude_gfx_buffer_ready( cmd->gpu, mesh->tangent_buffer )
    && crude_gfx_buffer_ready( cmd->gpu, mesh->normal_buffer )
    && crude_gfx_buffer_ready( cmd->gpu, mesh->texcoord_buffer )
    && crude_gfx_buffer_ready( cmd->gpu, mesh->index_buffer );

  if ( !mesh_buffers_ready )
  {
    return;
  }

  crude_gfx_descriptor_set_creation ds_creation = crude_gfx_descriptor_set_creation_empty();
  ds_creation.samplers[ 0 ] = CRUDE_GFX_SAMPLER_HANDLE_INVALID;
  ds_creation.samplers[ 1 ] = CRUDE_GFX_SAMPLER_HANDLE_INVALID;
  ds_creation.bindings[ 0 ] = 0;
  ds_creation.bindings[ 1 ] = 1;
  ds_creation.resources[ 0 ] = cmd->gpu->frame_buffer.index;
  ds_creation.resources[ 1 ] = mesh->material_buffer.index;
  ds_creation.num_resources = 2;
  ds_creation.layout = crude_gfx_access_pipeline( cmd->gpu, mesh->material->technique->passes[ 0 ].pipeline )->descriptor_set_layout_handle[ 0 ];

  crude_gfx_descriptor_set_handle descriptor_set = crude_gfx_cmd_create_local_descriptor_set( cmd, &ds_creation );

  crude_gfx_cmd_bind_vertex_buffer( cmd, mesh->position_buffer, 0, mesh->position_offset );
  crude_gfx_cmd_bind_vertex_buffer( cmd, mesh->tangent_buffer, 1, mesh->tangent_offset );
  crude_gfx_cmd_bind_vertex_buffer( cmd, mesh->normal_buffer, 2, mesh->normal_offset );
  crude_gfx_cmd_bind_vertex_buffer( cmd, mesh->texcoord_buffer, 3, mesh->texcoord_offset );
  crude_gfx_cmd_bind_index_buffer( cmd, mesh->index_buffer, mesh->index_offset );
  crude_gfx_cmd_bind_local_descriptor_set( cmd, descriptor_set );
  crude_gfx_cmd_draw_indexed( cmd, mesh->primitive_count, 1, 0, 0, 0 );
}

void
draw_scene_
(
  _In_ crude_gfx_cmd_buffer                               *cmd,
  _In_ crude_gfx_scene_renderer_geometry_pass             *pass
)
{
  crude_gfx_renderer_material *last_material = NULL;
  for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( pass->mesh_instances ); ++mesh_index )
  {
    crude_gfx_mesh_instance                               *mesh_instance;
    crude_gfx_mesh                                        *mesh;
  
    mesh_instance = &pass->mesh_instances[ mesh_index ];
    mesh = mesh_instance->mesh;
    if ( mesh->material != last_material )
    {
      crude_gfx_cmd_bind_pipeline( cmd, mesh->material->technique->passes[ mesh_instance->material_pass_index ].pipeline );
      last_material = mesh->material;
    }
    draw_mesh_( cmd, mesh );
  }
}

/************************************************
 *
 * GLTF Utils Functinos Declaration
 * 
 ***********************************************/
void
upload_gltf_to_scene_renderer_
(
  _In_ crude_gfx_scene_renderer                          *scene_renderer,
  _In_ char const                                        *gltf_path,
  _In_ char const                                        *resoruces_dir,
  _In_ crude_stack_allocator                             *temporary_allocator
)
{
  crude_gfx_renderer_material_creation                     material_creatoin;
  crude_allocator_container                                temporary_allocator_container;
  crude_string_buffer                                      temporary_string_buffer;
  cgltf_data                                              *gltf;
  cgltf_scene                                             *root_scene;
  uint32                                                   temporary_allocator_mark;
  char                                                     gltf_directory[ 512 ];
  size_t                                                   scene_renderer_images_offset;
  size_t                                                   scene_renderer_samplers_offset;
  size_t                                                   scene_renderer_buffers_offset;

  temporary_allocator_container = crude_stack_allocator_pack( temporary_allocator );
  temporary_allocator_mark = crude_stack_allocator_get_marker( temporary_allocator );

  crude_string_buffer_initialize( &temporary_string_buffer, 1024, temporary_allocator_container );
  
  /* Parse gltf */
  {
    cgltf_result                                           result;
    cgltf_options                                          gltf_options;

    gltf_options = ( cgltf_options ){ 
      .memory = {
        .alloc_func = temporary_allocator_container.allocate,
        .free_func  = temporary_allocator_container.deallocate,
        .user_data = temporary_allocator_container.ctx
      },
    };
    
    gltf = NULL;
    result = cgltf_parse_file( &gltf_options, gltf_path, &gltf );
    if ( result != cgltf_result_success )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to parse gltf file: %s", gltf_path );
    }
    
    result = cgltf_load_buffers( &gltf_options, gltf, gltf_path );
    if ( result != cgltf_result_success )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to load buffers from gltf file: %s", gltf_path );
    }
    
    result = cgltf_validate( gltf );
    if ( result != cgltf_result_success )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Failed to validate gltf file: %s", gltf_path );
    }
  }

  /* Get gltf directory */
  crude_memory_copy( gltf_directory, gltf_path, sizeof( gltf_directory ) );
  crude_file_directory_from_path( gltf_directory );

  scene_renderer_images_offset = CRUDE_ARRAY_LENGTH( scene_renderer->images );
  scene_renderer_samplers_offset = CRUDE_ARRAY_LENGTH( scene_renderer->samplers );
  scene_renderer_buffers_offset = CRUDE_ARRAY_LENGTH( scene_renderer->buffers );

  for ( uint32 image_index = 0; image_index < gltf->images_count; ++image_index )
  {
    crude_gfx_renderer_texture                            *texture_resource;
    crude_gfx_texture_creation                             texture_creation;
    cgltf_image const                                     *image;
    char                                                  *image_full_filename;
    int                                                    comp, width, height;
    
    image = &gltf->images[ image_index ];
    image_full_filename = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s%s", gltf_directory, image->uri );
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
      buffer_name = crude_string_buffer_append_use_f( &temporary_string_buffer, "scene_renderer_buffer%i", buffer_view_index );
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

    crude_string_buffer_clear( &temporary_string_buffer );
  }
  
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
      crude_gfx_mesh                                       mesh_draw;
      cgltf_primitive                                     *mesh_primitive;
      cgltf_accessor                                      *indices_accessor;
      cgltf_buffer_view                                   *indices_buffer_view;
      crude_gfx_renderer_buffer                           *indices_buffer_gpu;
      bool                                                 material_transparent;
      
      mesh_primitive = &node->mesh->primitives[ primitive_index ];

      mesh_draw = ( crude_gfx_mesh ){ 0 };
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

      material_transparent = create_gltf_mesh_material_( gltf, scene_renderer->renderer, scene_renderer, mesh_primitive->material, &mesh_draw, scene_renderer_images_offset, scene_renderer_samplers_offset );
      
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
  cgltf_free( gltf );
  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_mark );
}


/************************************************
 *
 * GLTF Utils Functinos Implementation
 * 
 ***********************************************/
bool
create_gltf_mesh_material_
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