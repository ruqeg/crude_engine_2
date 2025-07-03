#include <imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include <core/file.h>
#include <platform/platform_system.h>
#include <platform/platform_components.h>
#include <scene/free_camera_system.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>
#include <graphics/renderer_resources_loader.h>

#include <paprika.h>

CRUDE_ECS_SYSTEM_DECLARE( paprika_graphics_system_ );
CRUDE_ECS_SYSTEM_DECLARE( paprika_update_system_ );

static void
paprika_graphics_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
paprika_update_system_
(
  _In_ ecs_iter_t                                         *it
);

static void
paprika_graphics_initialize_
(
  _In_ crude_paprika                                      *paprika,
  _In_ crude_window_handle                                 window_handle
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
  
  paprika->platform_node = crude_entity_create_empty( paprika->engine->world, "paprika" );
  CRUDE_ENTITY_SET_COMPONENT( paprika->platform_node, crude_window, { 
    .width     = 800,
    .height    = 600,
    .maximized = false
  });
  CRUDE_ENTITY_SET_COMPONENT( paprika->platform_node, crude_input, { 0 } );
  
  /* Create scene */
  {
    crude_scene_creation creation = {
      .world = paprika->engine->world,
      .input_entity = paprika->platform_node,
      .resources_path = "\\..\\..\\resources\\",
      .temporary_allocator = &paprika->temporary_allocator,
      .allocator_container = crude_heap_allocator_pack( &paprika->graphics_allocator ), 
    };
    crude_scene_initialize( &paprika->scene, &creation );
  }
  crude_scene_load( &paprika->scene, "scene.json" );
  
  IMGUI_CHECKVERSION();
  paprika->imgui_context = ImGui::CreateContext();

  /* Create Graphics */
  {
    crude_window_handle *window_handle = CRUDE_ENTITY_GET_MUTABLE_COMPONENT( paprika->platform_node, crude_window_handle );
    paprika_graphics_initialize_( paprika, *window_handle );
    CRUDE_ECS_SYSTEM_DEFINE( paprika->engine->world, paprika_graphics_system_, EcsPreStore, paprika, {
      { .id = ecs_id( crude_transform ) },
      { .id = ecs_id( crude_free_camera ) },
    } );
  }
  
  CRUDE_ECS_SYSTEM_DEFINE( paprika->engine->world, paprika_update_system_, EcsOnUpdate, NULL, {
    { .id = ecs_id( crude_input ) },
    { .id = ecs_id( crude_window_handle ) },
  } );
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
  crude_entity_destroy( paprika->platform_node );
  crude_scene_deinitialize( &paprika->scene );
  paprika_graphics_deinitialize_( paprika );
  crude_heap_allocator_deinitialize( &paprika->graphics_allocator );
  crude_stack_allocator_deinitialize( &paprika->temporary_allocator );

  ImGui::DestroyContext( ( ImGuiContext* )paprika->imgui_context );
}

void
crude_paprika_update
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_input const *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( paprika->platform_node, crude_input );
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
      .sdl_window             = CRUDE_REINTERPRET_CAST( SDL_Window*, window_handle.value ),
      .vk_application_name    = "CrudeEngine",
      .vk_application_version = VK_MAKE_VERSION( 1, 0, 0 ),
      .allocator_container    = paprika->graphics.allocator_container,
      .temporary_allocator    = &paprika->temporary_allocator,
      .queries_per_frame      = 1u,
      .num_threads            = CRUDE_STATIC_CAST( uint16, enkiGetNumTaskThreads( CRUDE_REINTERPRET_CAST( enkiTaskScheduler*, paprika->graphics.asynchronous_loader_manager->task_sheduler ) ) ),
    };
    crude_gfx_device_initialize( &paprika->graphics.gpu, &device_creation );
  }
  
  /* Create Renderer */
  {
    crude_gfx_renderer_creation renderer_creation = {
      .gpu                 = &paprika->graphics.gpu,
      .allocator_container = paprika->graphics.allocator_container,
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
  
  /* Parse render graph*/
  {
    crude_string_buffer                                    temporary_name_buffer;
    char                                                   working_directory[ 512 ];
    char const                                            *render_graph_file_path;

    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    crude_string_buffer_initialize( &temporary_name_buffer, 1024, crude_stack_allocator_pack( &paprika->temporary_allocator ) );
  
    render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\render_graph.json", "\\..\\..\\resources\\render_graph.json" );
    crude_gfx_render_graph_parse_from_file( &paprika->graphics.render_graph, render_graph_file_path, &paprika->temporary_allocator );
    crude_gfx_render_graph_compile( &paprika->graphics.render_graph );
  }
  
  /* Create Render Tecnhique & Renderer Passes*/
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\resources\\main_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );
  crude_gfx_renderer_technique_load_from_file( "\\..\\..\\resources\\meshlet_technique.json", &paprika->graphics.renderer, &paprika->graphics.render_graph, &paprika->temporary_allocator );

  /* Create Scene Renderer */
  {
    crude_gfx_scene_renderer_creation creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_creation );
    creation.node = paprika->scene.main_node;
    creation.renderer = &paprika->graphics.renderer;
    creation.async_loader = &paprika->graphics.async_loader;
    creation.allocator_container = paprika->graphics.allocator_container;
    creation.temporary_allocator = &paprika->temporary_allocator;
    creation.task_scheduler = paprika->graphics.asynchronous_loader_manager->task_sheduler;
    creation.imgui_context = paprika->imgui_context;
    crude_gfx_scene_renderer_initialize( &paprika->graphics.scene_renderer, &creation );
    crude_gfx_scene_renderer_register_passes( &paprika->graphics.scene_renderer, &paprika->graphics.render_graph );
  }
  
  crude_stack_allocator_free_marker( &paprika->temporary_allocator, temporary_allocator_marker );
}

void
paprika_graphics_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_paprika *paprika = ( crude_paprika* )it->ctx;

  crude_gfx_new_frame( &paprika->graphics.gpu );
  
  ImGui::SetCurrentContext( ( ImGuiContext* ) paprika->imgui_context );
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  if ( paprika->graphics.gpu.swapchain_resized_last_frame )
  {
    crude_gfx_render_graph_on_resize( &paprika->graphics.render_graph, paprika->graphics.gpu.vk_swapchain_width, paprika->graphics.gpu.vk_swapchain_height );
  }
  
  /* Update frame buffer */
  {
    crude_gfx_map_buffer_parameters frame_buffer_map = { paprika->graphics.scene_renderer.scene_cb, 0 };
    crude_gfx_scene_constant *scene_data = CRUDE_REINTERPRET_CAST( crude_gfx_scene_constant*, crude_gfx_map_buffer( &paprika->graphics.gpu, &frame_buffer_map ) );
    if ( scene_data )
    {
      crude_camera const *camera = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( paprika->scene.main_camera, crude_camera );
      crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( paprika->scene.main_camera, crude_transform );
    
      crude_matrix view_to_world = crude_transform_node_to_world( paprika->scene.main_camera, transform );
      crude_matrix world_to_view = crude_mat_inverse( NULL, view_to_world );
      crude_matrix view_to_clip = crude_camera_view_to_clip( camera );
      crude_matrix clip_to_view = crude_mat_inverse( NULL, view_to_clip );
      
      crude_store_float4x4a( &scene_data->clip_to_view, clip_to_view ); 
      crude_store_float4x4a( &scene_data->view_to_clip, view_to_clip ); 
      crude_store_float4x4a( &scene_data->view_to_world, view_to_world ); 
      crude_store_float4x4a( &scene_data->world_to_view, world_to_view ); 
      crude_gfx_unmap_buffer( &paprika->graphics.gpu, frame_buffer_map.buffer );
    }
  }
  
  /* Update mesh buffer */
  {
    for ( uint32 mesh_index = 0; mesh_index < CRUDE_ARRAY_LENGTH( paprika->graphics.scene_renderer.meshes ); ++mesh_index )
    {
      crude_gfx_mesh_cpu *mesh_draw = &paprika->graphics.scene_renderer.meshes[ mesh_index ];
  
      crude_gfx_map_buffer_parameters mesh_buffer_map = { mesh_draw->material_buffer, 0, 0 };
      crude_gfx_shader_mesh_constants *mesh_data = CRUDE_REINTERPRET_CAST( crude_gfx_shader_mesh_constants*, crude_gfx_map_buffer( &paprika->graphics.gpu, &mesh_buffer_map ) );
      if ( mesh_data )
      {
        mesh_data->textures.x = mesh_draw->albedo_texture_index;
        mesh_data->textures.y = mesh_draw->roughness_texture_index;
        mesh_data->textures.z = mesh_draw->normal_texture_index;
        mesh_data->textures.w = mesh_draw->occlusion_texture_index;
        mesh_data->albedo_color_factor = CRUDE_COMPOUNT( crude_float4a, { mesh_draw->albedo_color_factor.x, mesh_draw->albedo_color_factor.y, mesh_draw->albedo_color_factor.z, mesh_draw->albedo_color_factor.w } );
        mesh_data->metallic_roughness_occlusion_factor = CRUDE_COMPOUNT( crude_float3a, { mesh_draw->metallic_roughness_occlusion_factor.x, mesh_draw->metallic_roughness_occlusion_factor.y, mesh_draw->metallic_roughness_occlusion_factor.z } );
        mesh_data->alpha_cutoff.x = mesh_draw->alpha_cutoff;
        mesh_data->flags.x = mesh_draw->flags;
        
        crude_transform const *transform = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( mesh_draw->node, crude_transform );
        crude_matrix model_to_world = crude_transform_node_to_world( paprika->scene.main_camera, transform );
        crude_store_float4x4a( &mesh_data->model_to_world, model_to_world ); 
      
        crude_gfx_unmap_buffer( &paprika->graphics.gpu, mesh_draw->material_buffer );
      }
    }
  }

  ImGui::Begin("sdfsdf");
  ImGui::End();
  
  crude_gfx_scene_renderer_submit_draw_task( &paprika->graphics.scene_renderer, false );
  
  crude_gfx_renderer_add_texture_update_commands( &paprika->graphics.renderer, 0 );

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
  crude_gfx_scene_renderer_deinitialize( &paprika->graphics.scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &paprika->graphics.async_loader );
  crude_gfx_render_graph_deinitialize( &paprika->graphics.render_graph );
  crude_gfx_render_graph_builder_deinitialize( &paprika->graphics.render_graph_builder );
  crude_gfx_renderer_deinitialize( &paprika->graphics.renderer );
  crude_gfx_device_deinitialize( &paprika->graphics.gpu );
}

void
paprika_update_system_
(
  _In_ ecs_iter_t                                         *it
)
{
  crude_input *input_per_entity = ecs_field( it, crude_input, 0 );
  crude_window_handle *window_handle_per_entity = ecs_field( it, crude_window_handle, 1 );
  
  for ( uint32 i = 0; i < it->count; ++i )
  {
    crude_input *input = &input_per_entity[ i ];
    crude_window_handle *window_handle = &window_handle_per_entity[ i ];
    
    if ( input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      SDL_HideCursor();
    }
    if ( !input->mouse.right.current && input->mouse.right.current != input->prev_mouse.right.current )
    {
      SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle ), input->mouse.wnd.x, input->mouse.wnd.y );
      SDL_ShowCursor();
    }
    else if ( input->mouse.wnd.x || input->mouse.wnd.y )
    {
      if ( !SDL_CursorVisible() )
      {
        SDL_WarpMouseInWindow( CRUDE_CAST( SDL_Window*, window_handle ), input->mouse.wnd.x, input->mouse.wnd.y );
      }
    }
  }
}