#include <engine/core/time.h>
#include <engine/core/profiler.h>
#include <engine/graphics/gpu_resources_loader.h>

#include <engine/engine/graphics_thread_manager.h>

static void
crude_graphics_thread_manager_pinned_task_graphics_loop_
(
  _In_ void                                               *ctx
);

void
crude_graphics_thread_manager_initialize
(
  _In_ crude_graphics_thread_manager                      *manager,
  _In_ crude_environment const                            *environment,
  _In_ SDL_Window                                         *sdl_window,
  _In_ crude_task_sheduler                                *task_sheduler,
  _In_ crude_gfx_asynchronous_loader_manager              *___asynchronous_loader_manager,
  _In_ crude_scene_thread_manager                         *___scene_thread_manager,
  _In_ ImGuiContext                                       *imgui_context,
  _In_ crude_heap_allocator                               *cgltf_temporary_allocator,
  _In_ crude_stack_allocator                              *model_renderer_resources_manager_temporary_allocator,
  _In_ crude_heap_allocator                               *common_allocator,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  char const                                              *render_graph_file_path;
  crude_string_buffer                                      temporary_name_buffer;
  crude_gfx_model_renderer_resources_manager_creation      model_renderer_resources_manager_creation;
  crude_gfx_device_creation                                device_creation;
  crude_gfx_scene_renderer_creation                        scene_renderer_creation;
  uint32                                                   temporary_allocator_marker;
  
  manager->___asynchronous_loader_manager = ___asynchronous_loader_manager;
  manager->___scene_thread_manager = ___scene_thread_manager;
  manager->imgui_context = imgui_context;

  temporary_allocator_marker = crude_stack_allocator_get_marker( temporary_allocator );
  
  crude_string_buffer_initialize( &temporary_name_buffer, 1024, crude_stack_allocator_pack( temporary_allocator ) );

  device_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_device_creation );
  device_creation.sdl_window = sdl_window;
  device_creation.vk_application_name = "CrudeEngine";
  device_creation.vk_application_version = VK_MAKE_VERSION( 1, 0, 0 );
  device_creation.allocator_container = crude_heap_allocator_pack( common_allocator );
  device_creation.temporary_allocator = temporary_allocator;
  device_creation.queries_per_frame = 1u;
  device_creation.num_threads = 3;
  device_creation.shaders_absolute_directory = environment->directories.shaders_absolute_directory;
  device_creation.techniques_absolute_directory = environment->directories.techniques_absolute_directory;
  device_creation.compiled_shaders_absolute_directory = environment->directories.compiled_shaders_absolute_directory;
  crude_gfx_device_initialize( &manager->gpu, &device_creation );
  
  crude_gfx_render_graph_builder_initialize( &manager->render_graph_builder, &manager->gpu );
  crude_gfx_render_graph_initialize( &manager->render_graph, &manager->render_graph_builder );
  
  crude_gfx_asynchronous_loader_initialize( &manager->async_loader, &manager->gpu );
  crude_gfx_asynchronous_loader_manager_add_loader( ___asynchronous_loader_manager, &manager->async_loader );

//#if CRUDE_DEVELOP
  render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", environment->directories.render_graph_absolute_directory, "render_graph.json" );
//#else
//  render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", game->render_graph_absolute_directory, "game\\render_graph_production.json" );
//#endif
  crude_gfx_render_graph_parse_from_file( &manager->render_graph, render_graph_file_path, temporary_allocator );
  crude_gfx_render_graph_compile( &manager->render_graph, temporary_allocator );
  
  if ( manager->gpu.mesh_shaders_extension_present )
  {
    crude_gfx_technique_load_from_file( "deferred_meshlet.json", &manager->gpu, &manager->render_graph, temporary_allocator );
  }
  else
  {
    crude_gfx_technique_load_from_file( "deferred_classic.json", &manager->gpu, &manager->render_graph, temporary_allocator );
  }
  //crude_gfx_technique_load_from_file( "pointshadow_meshlet.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "compute.json", &manager->gpu, &manager->render_graph, temporary_allocator );
  crude_gfx_technique_load_from_file( "debug.json", &manager->gpu, &manager->render_graph, temporary_allocator );
  crude_gfx_technique_load_from_file( "fullscreen.json", &manager->gpu, &manager->render_graph, temporary_allocator );
  crude_gfx_technique_load_from_file( "imgui.json", &manager->gpu, &manager->render_graph, temporary_allocator );
  crude_gfx_technique_load_from_file( "game/fullscreen.json", &manager->gpu, &manager->render_graph, temporary_allocator );
  
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_renderer_technique_load_from_file( "ray_tracing_solid.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
  
  model_renderer_resources_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_manager_creation );
  model_renderer_resources_manager_creation.allocator = common_allocator;
  model_renderer_resources_manager_creation.async_loader = &manager->async_loader;
  model_renderer_resources_manager_creation.cgltf_temporary_allocator = cgltf_temporary_allocator;
  model_renderer_resources_manager_creation.temporary_allocator = model_renderer_resources_manager_temporary_allocator;
  crude_gfx_model_renderer_resources_manager_intialize( &manager->model_renderer_resources_manager, &model_renderer_resources_manager_creation );

  scene_renderer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_creation );
  scene_renderer_creation.async_loader = &manager->async_loader;
  scene_renderer_creation.allocator = common_allocator;
  scene_renderer_creation.temporary_allocator = temporary_allocator;
  scene_renderer_creation.model_renderer_resources_manager = &manager->model_renderer_resources_manager;
  scene_renderer_creation.imgui_pass_enalbed = true;
  scene_renderer_creation.imgui_context = imgui_context;
  crude_gfx_scene_renderer_initialize( &manager->scene_renderer, &scene_renderer_creation );

#if CRUDE_DEVELOP
  manager->scene_renderer.options.hide_collision = true;
  manager->scene_renderer.options.hide_debug_gltf = true;
#endif

  manager->scene_renderer.options.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 1, 1, 1 } );
  manager->scene_renderer.options.ambient_intensity = 1.5f;
  manager->scene_renderer.options.background_intensity = 0.f;
  manager->scene_renderer.options.hdr_pre_tonemapping_texture_name = "pbr";

  manager->framerate = 120;

  manager->absolute_time = 0.f;

  manager->imgui_context = imgui_context;

  manager->running = true;

  mtx_init( &manager->mutex, mtx_plain );
  
  crude_gfx_scene_renderer_rebuild_light_gpu_buffers( &manager->scene_renderer );

  crude_gfx_scene_renderer_register_passes( &manager->scene_renderer, &manager->render_graph );
  //crude_gfx_render_graph_builder_register_render_pass( engine->render_graph.builder, "game_postprocessing_pass", crude_gfx_game_postprocessing_pass_pack( &game->game_postprocessing_pass ) );
  
  crude_task_sheduler_add_pinned_task( task_sheduler, crude_graphics_thread_manager_pinned_task_graphics_loop_, manager, CRUDE_GRAPHICS_ACTIVE_THREAD  );  

  crude_stack_allocator_free_marker( temporary_allocator, temporary_allocator_marker );
}

void
crude_graphics_thread_manager_deinitialize
(
  _In_ crude_graphics_thread_manager                      *manager
)
{
  manager->running = false;
  mtx_destroy( &manager->mutex );
  vkDeviceWaitIdle( manager->gpu.vk_device );
  crude_gfx_asynchronous_loader_manager_remove_loader( manager->___asynchronous_loader_manager, &manager->async_loader );
  crude_gfx_scene_renderer_deinitialize( &manager->scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &manager->async_loader );
  crude_gfx_model_renderer_resources_manager_deintialize( &manager->model_renderer_resources_manager );
  crude_gfx_render_graph_builder_deinitialize( &manager->render_graph_builder );
  crude_gfx_render_graph_deinitialize( &manager->render_graph );
  crude_gfx_device_deinitialize( &manager->gpu );
}

void
crude_graphics_thread_manager_stop
(
  _In_ crude_graphics_thread_manager                      *manager
)
{
  mtx_lock( &manager->mutex );
  manager->running = false;
  mtx_unlock( &manager->mutex );
}

void
crude_graphics_thread_manager_pinned_task_graphics_loop_
(
  _In_ void                                               *ctx
)
{
  crude_graphics_thread_manager *manager = CRUDE_REINTERPRET_CAST( crude_graphics_thread_manager*, ctx );
  
  CRUDE_PROFILER_SET_THREAD_NAME( "crude_engine_pinned_task_graphics_loop_" );

  while ( manager->running )
  {
    crude_gfx_texture                                       *final_render_texture;
    crude_ecs                                               *world;
    float32                                                  last_graphics_update_delta;
    bool                                                     new_buffers_recrteated_or_model_initialized;
  
    CRUDE_PROFILER_ZONE_NAME( "crude_engine_pinned_task_graphics_loop_" );
  
    mtx_lock( &manager->mutex );
    
    final_render_texture = crude_gfx_access_texture( &manager->gpu, crude_gfx_render_graph_builder_access_resource_by_name( manager->scene_renderer.render_graph->builder, CRUDE_GRAPHICS_PRESENT_TEXTURE_NAME )->resource_info.texture.handle );

    last_graphics_update_delta = crude_time_delta_seconds( manager->last_graphics_update_time, crude_time_now( ) );
    
    if ( last_graphics_update_delta < 1.f / manager->framerate )
    {
        goto cleanup;
    }

    world = crude_scene_thread_manager_lock_world( manager->___scene_thread_manager );
    {
      crude_entity camera_node = crude_scene_thread_manager_get_camera_node_UNSAFE( manager->___scene_thread_manager );
      crude_entity main_node = crude_scene_thread_manager_get_main_node_UNSAFE( manager->___scene_thread_manager );

      if ( !crude_entity_valid( world, camera_node ) )
      {
        crude_scene_thread_manager_unlock_world( manager->___scene_thread_manager );
        goto cleanup;
      }
  
      new_buffers_recrteated_or_model_initialized = crude_gfx_scene_renderer_update_instances_from_node( &manager->scene_renderer, world, main_node );
      manager->scene_renderer.options.camera = *CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, camera_node, crude_camera );
      XMStoreFloat4x4( &manager->scene_renderer.options.camera_view_to_world, crude_transform_node_to_world( world, camera_node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( world, camera_node, crude_transform ) ) );
    }
    crude_scene_thread_manager_unlock_world( manager->___scene_thread_manager );

    manager->absolute_time += last_graphics_update_delta;
  
    manager->last_graphics_update_time = crude_time_now( );

    manager->scene_renderer.options.absolute_time = manager->absolute_time;

    crude_gfx_new_frame( &manager->gpu );
  
    if ( new_buffers_recrteated_or_model_initialized )
    {
      CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Model being loaded during scene rendering!" );
      crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &manager->model_renderer_resources_manager );
    }
 
    ImGui::SetCurrentContext( manager->imgui_context );
    ImGui_ImplSDL3_NewFrame( );
    ImGui::NewFrame( );
#if CRUDE_DEVELOP
    //crude_devmenu_draw( &game->devmenu );
#endif
  
    if ( manager->gpu.swapchain_resized_last_frame )
    {
      crude_gfx_scene_renderer_on_resize( &manager->scene_renderer );
      crude_gfx_render_graph_on_resize( &manager->render_graph, manager->gpu.vk_swapchain_width, manager->gpu.vk_swapchain_height );
    }

    crude_gfx_scene_renderer_submit_draw_task( &manager->scene_renderer, false );

    crude_gfx_present( &manager->gpu, final_render_texture );

cleanup:
    mtx_unlock( &manager->mutex );
    CRUDE_PROFILER_ZONE_END;
  }
}