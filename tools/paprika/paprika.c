#include <engine.h>
#include <core/ecs_utils.h>
#include <platform/platform_system.h>
#include <platform/platform_components.h>
#include <scene/free_camera_system.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>

#include <paprika.h>

static void
paprika_graphics_initialize_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_window_handle                                 window_handle
);

static void
paprika_graphics_process_
(
  _In_ ecs_iter_t                                         *it
);

static void
paprika_graphics_deinitialize_
(
  _In_ crude_paprika                                      *paprika
);

void
crude_paprika_initialize
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_engine                                       *engine
)
{
  paprika->engine = engine;
  paprika->working = true;

  crude_heap_allocator_initialize( &paprika->graphics_allocator, CRUDE_RMEGA( 32 ), "renderer_allocator" );
  crude_stack_allocator_initialize( &paprika->temporary_allocator, CRUDE_RMEGA( 32 ), "temprorary_allocator" );
  
  ECS_IMPORT( paprika->engine->world, crude_platform_system );
  ECS_IMPORT( paprika->engine->world, crude_free_camera_system );
  
  paprika->scene = crude_entity_create_empty( paprika->engine->world, "paprika" );

  {
    crude_scene_creation creation = {
      .allocator_container = crude_heap_allocator_pack( &paprika->graphics_allocator ), 
      .resources_path = "\\..\\..\\resources\\",
      .temporary_allocator = &paprika->temporary_allocator,
      .world = paprika->engine->world,
      .input_entity = paprika->scene,
    };
    crude_scene_initialize( &paprika->scenee, &creation );
  }

  crude_scene_load( &paprika->scenee, "scene.json" );

  /* Create free camera */
  paprika->camera = crude_entity_create_empty( paprika->engine->world, "camera1" );
  CRUDE_ENTITY_SET_COMPONENT( paprika->camera, crude_camera, {
    .fov_radians = CRUDE_CPI4,
    .near_z = 0.01,
    .far_z = 1000,
    .aspect_ratio = 4.0 / 3.0 } );
  CRUDE_ENTITY_SET_COMPONENT( paprika->camera, crude_transform, {
    .translation = { 0, 0, -5 },
    .rotation = { 0, 0, 0, 1 },
    .scale = { 1, 1, 1 }, } );
  CRUDE_ENTITY_SET_COMPONENT( paprika->camera, crude_free_camera, {
    .moving_speed_multiplier = { 7.0, 7.0, 7.0 },
    .rotating_speed_multiplier  = { -0.25f, -0.25f },
    .entity_input = paprika->scene } );
  
  /* Create scene */
  CRUDE_ENTITY_SET_COMPONENT( paprika->scene, crude_window, { 
    .width     = 800,
    .height    = 600,
    .maximized = false });
  CRUDE_ENTITY_SET_COMPONENT( paprika->scene, crude_scene, { 0 } );
  CRUDE_ENTITY_SET_COMPONENT( paprika->scene, crude_input, { 0 } );

  crude_window_handle *window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->scene, crude_window_handle );
  paprika_graphics_initialize_( paprika, *window_handle );

  ecs_system( paprika->engine->world, {
    .entity = ecs_entity( paprika->engine->world, { .name = "paprika_graphics_system", .add = ecs_ids( ecs_dependson( crude_on_render_tag ) ) } ),
    .callback = paprika_graphics_process_,
    .ctx = paprika } );
}

void
crude_paprika_deinitialize
(
  _In_ crude_paprika                                      *paprika
)
{
  if ( !paprika->working )
  {
    return;
  }

  paprika->working = false;
  crude_entity_destroy( paprika->camera );
  crude_entity_destroy( paprika->scene );
  crude_scene_deinitialize( &paprika->scenee );
  paprika_graphics_deinitialize_( paprika );
  crude_heap_allocator_deinitialize( &paprika->graphics_allocator );
  crude_stack_allocator_deinitialize( &paprika->temporary_allocator );
}

bool
crude_paprika_update
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_input *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( paprika->scene, crude_input );
  if ( input && input->should_close_window )
  {
    crude_paprika_deinitialize( paprika );
  }
}

static void
paprika_graphics_initialize_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_window_handle                                 window_handle
)
{
  uint32                                                   temporary_allocator_marker;

  temporary_allocator_marker = crude_stack_allocator_get_marker( &paprika->temporary_allocator );
  
  paprika->graphics.asynchronous_loader_manager = &paprika->engine->asynchronous_loader_manager;
  paprika->graphics.allocator_container = crude_heap_allocator_pack( &paprika->graphics_allocator );
  
  /* Create Device */
  {
    crude_gfx_device_creation device_creation = {
      .sdl_window             = window_handle.value,
      .vk_application_name    = "CrudeEngine",
      .vk_application_version = VK_MAKE_VERSION( 1, 0, 0 ),
      .allocator_container    = paprika->graphics.allocator_container,
      .queries_per_frame      = 1u,
      .num_threads            = enkiGetNumTaskThreads( paprika->graphics.asynchronous_loader_manager->task_sheduler ),
      .temporary_allocator    = &paprika->temporary_allocator
    };
    crude_gfx_device_initialize( &paprika->graphics.gpu, &device_creation );
  }
  
  /* Create Renderer */
  {
    crude_gfx_renderer_creation renderer_creation = {
      .allocator_container = paprika->graphics.allocator_container,
      .gpu                 = &paprika->graphics.gpu
    };
    crude_gfx_renderer_initialize( &paprika->graphics.renderer, &renderer_creation );
  }    
  
  /* Create Render Graph */
  crude_gfx_render_graph_builder_initialize( &paprika->graphics.render_graph_builder, &paprika->graphics.gpu );
  crude_gfx_render_graph_initialize( &paprika->graphics.render_graph, &paprika->graphics.render_graph_builder );
  
  /* Create Async Loader */
  {
    crude_gfx_asynchronous_loader_initialize( &paprika->graphics.async_loader, &paprika->graphics.renderer );
    crude_gfx_asynchronous_loader_manager_add_loader( paprika->graphics.asynchronous_loader_manager, &paprika->graphics.async_loader );
  }
  
  {
    crude_gfx_buffer_creation frame_buffer_creation = {
      .type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC ,
      .size = sizeof( crude_gfx_frame_buffer_data ),
      .name = "FrameBuffer",
    };
    paprika->graphics.gpu.frame_buffer = crude_gfx_create_buffer( &paprika->graphics.gpu, &frame_buffer_creation );
  }
  
  paprika->graphics.camera = paprika->camera;
  
  crude_stack_allocator_free_marker( &paprika->temporary_allocator, temporary_allocator_marker );
}

void
paprika_graphics_process_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_paprika *paprika = ( crude_paprika* )it->ctx;

  crude_gfx_new_frame( &paprika->graphics.gpu );
  
  if ( paprika->graphics.gpu.swapchain_resized_last_frame )
  {
    crude_gfx_render_graph_on_resize( &paprika->graphics.render_graph, paprika->graphics.gpu.vk_swapchain_width, paprika->graphics.gpu.vk_swapchain_height );
  }
  
  /* Update fame buffer */
  {
    crude_gfx_map_buffer_parameters frame_buffer_map = { paprika->graphics.gpu.frame_buffer, 0, 0 };
    crude_gfx_frame_buffer_data *frame_buffer_data = crude_gfx_map_buffer( &paprika->graphics.gpu, &frame_buffer_map );
    if ( frame_buffer_data )
    {
      crude_camera const *camera = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( paprika->graphics.camera, crude_camera );
      crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( paprika->graphics.camera, crude_transform );
    
      crude_matrix world_to_view = crude_mat_inverse( NULL, crude_transform_node_to_world( paprika->graphics.camera, transform ) );
      crude_matrix view_to_clip = crude_camera_view_to_clip( camera );
      
      crude_store_float4x4a( &frame_buffer_data->world_to_view, world_to_view ); 
      crude_store_float4x4a( &frame_buffer_data->view_to_clip, view_to_clip ); 
      crude_gfx_unmap_buffer( &paprika->graphics.gpu, paprika->graphics.gpu.frame_buffer );
    }
  }
  
  /* update mesh buffer */
  {
    for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( paprika->graphics.scene.meshes ); ++mesh_index )
    {
      crude_gfx_mesh *mesh_draw = &paprika->graphics.scene.meshes[ mesh_index ];
  
      crude_gfx_map_buffer_parameters mesh_buffer_map = { mesh_draw->material_buffer, 0, 0 };
      crude_gfx_shader_mesh_constants *mesh_data = crude_gfx_map_buffer( &paprika->graphics.gpu, &mesh_buffer_map );
      if ( mesh_data )
      {
        mesh_data->textures.x = mesh_draw->albedo_texture_index;
        mesh_data->textures.y = mesh_draw->roughness_texture_index;
        mesh_data->textures.z = mesh_draw->normal_texture_index;
        mesh_data->textures.w = mesh_draw->occlusion_texture_index;
        mesh_data->base_color_factor = ( crude_float4a ){ mesh_draw->base_color_factor.x, mesh_draw->base_color_factor.y, mesh_draw->base_color_factor.z, mesh_draw->base_color_factor.w } ;
        mesh_data->metallic_roughness_occlusion_factor = ( crude_float3a ){ mesh_draw->metallic_roughness_occlusion_factor.x, mesh_draw->metallic_roughness_occlusion_factor.y, mesh_draw->metallic_roughness_occlusion_factor.z };
        mesh_data->alpha_cutoff.x = mesh_draw->alpha_cutoff;
        mesh_data->flags.x = mesh_draw->flags;
        
        crude_transform model_transform = {
          .translation = { 0.0, 0.0, -4.0 },
          .rotation = { 0.0, 0.0, 0.0, 0.0 },
          .scale = { 0.3, 0.3, 0.3 },
        };
        crude_matrix model_to_world = crude_transform_node_to_world( paprika->graphics.camera, &model_transform );
        crude_store_float4x4a( &mesh_data->modelToWorld, model_to_world ); 
      
        crude_gfx_unmap_buffer( &paprika->graphics.gpu, mesh_draw->material_buffer );
      }
    }
  }
  
  crude_gfx_scene_renderer_submit_draw_task( &paprika->graphics.scene, paprika->graphics.asynchronous_loader_manager->task_sheduler, true );
  
  {
    crude_gfx_render_graph_node *final_render_graph_node = crude_gfx_render_graph_builder_access_node_by_name( &paprika->graphics.render_graph_builder, "geometry_pass" );
    crude_gfx_framebuffer *final_render_framebuffer = crude_gfx_access_framebuffer( &paprika->graphics.gpu, final_render_graph_node->framebuffer );
    crude_gfx_texture *final_render_texture = crude_gfx_access_texture( &paprika->graphics.gpu, final_render_framebuffer->color_attachments[ 0 ] );
    crude_gfx_present( &paprika->graphics.gpu, final_render_texture );
  }
}

void
paprika_graphics_deinitialize_
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_gfx_asynchronous_loader_manager_remove_loader( paprika->graphics.asynchronous_loader_manager, &paprika->graphics.async_loader );
  vkDeviceWaitIdle( paprika->graphics.gpu.vk_device );
  crude_gfx_destroy_buffer( &paprika->graphics.gpu, paprika->graphics.gpu.frame_buffer );
  crude_gfx_scene_renderer_deinitialize( &paprika->graphics.scene );
  crude_gfx_asynchronous_loader_deinitialize( &paprika->graphics.async_loader );
  crude_gfx_render_graph_deinitialize( &paprika->graphics.render_graph );
  crude_gfx_render_graph_builder_deinitialize( &paprika->graphics.render_graph_builder );
  crude_gfx_renderer_deinitialize( &paprika->graphics.renderer );
  crude_gfx_device_deinitialize( &paprika->graphics.gpu );
}