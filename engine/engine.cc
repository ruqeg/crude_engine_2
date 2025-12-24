#include <engine/core/profiler.h>
#include <engine/core/time.h>
#include <engine/core/array.h>
#include <engine/core/log.h>
#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/physics/physics_debug_system.h>

#include <engine/engine.h>

static void
crude_engine_initialize_services_
(
);

static void
crude_engine_deinitialize_services_
(
);

static void
crude_engine_initialize_allocators_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_allocators_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_ecs_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_ecs_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_tash_sheduler_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_task_sheduler_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_platform_
(
  _In_ crude_engine                                       *engine,
  _In_ char const                                         *window_title,
  _In_ uint64                                              window_width,
  _In_ uint64                                              window_height
);

static void
crude_engine_deinitialize_platform_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_imgui_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_imgui_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_debug_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_debug_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_audio_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_audio_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_graphics_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_graphics_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_collisions_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_collisions_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_physics_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_physics_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_initialize_scene_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_scene_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

static bool
crude_engine_parse_json_to_component_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name,
  _In_ crude_node_manager                                 *manager
);

static void
crude_engine_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json,
  _In_ crude_node_manager                                 *manager
);

static void
crude_engine_pinned_task_platform_loop_
(
  _In_ void                                               *ctx
);

static void
crude_engine_pinned_task_graphics_loop_
(
  _In_ void                                               *ctx
);

void
crude_engine_initialize
(
  _In_ crude_engine                                       *engine,
  _In_ crude_engine_creation const                        *creation
)
{
  *engine = CRUDE_COMPOUNT_EMPTY( crude_engine );
  crude_engine_initialize_services_( );
  crude_engine_initialize_allocators_( engine );
  crude_engine_initialize_ecs_( engine );
  crude_engine_initialize_tash_sheduler_( engine );
  crude_engine_initialize_platform_( engine, creation->window.title, creation->window.width, creation->window.height );
  crude_engine_initialize_imgui_( engine );
  crude_engine_initialize_debug_( engine );
  crude_engine_initialize_audio_( engine );
  crude_engine_initialize_graphics_( engine );
  crude_engine_initialize_collisions_( engine );
  crude_engine_initialize_physics_( engine );
  crude_engine_initialize_scene_( engine );
}

void
crude_engine_deinitialize
(
  _In_ crude_engine                                       *engine
)
{
  crude_engine_deinitialize_scene_( engine );
  crude_engine_deinitialize_physics_( engine );
  crude_engine_deinitialize_collisions_( engine );
  crude_engine_deinitialize_graphics_( engine );
  crude_engine_deinitialize_audio_( engine );
  crude_engine_deinitialize_debug_( engine );
  crude_engine_deinitialize_imgui_( engine );
  crude_engine_deinitialize_platform_( engine );
  crude_engine_deinitialize_task_sheduler_( engine );
  crude_engine_deinitialize_ecs_( engine );
  crude_engine_deinitialize_allocators_( engine );
  crude_engine_deinitialize_services_( );
}

bool
crude_engine_update
(
  _In_ crude_engine                                       *engine
)
{
  int64                                                    current_time;
  float32                                                  delta_time;
  bool                                                     should_not_quit;

  CRUDE_PROFILER_ZONE_NAME( "crude_engine_update" );
  
  should_not_quit = true;
  if ( crude_ecs_should_quit( engine->world ) )
  {
    should_not_quit = false;
    goto cleanup;
  }
  
  current_time = crude_time_now();
  delta_time = crude_time_delta_seconds( engine->last_update_time, current_time );
  crude_ecs_progress( engine->world, delta_time );
  engine->last_update_time = current_time;

cleanup:
  CRUDE_PROFILER_ZONE_END;
  return true;
}

void
crude_engine_initialize_services_
(
)
{
  crude_log_initialize( );
  crude_time_service_initialize( );
  crude_platform_service_initialize( );
}

static void
crude_engine_deinitialize_services_
(
)
{
  crude_platform_service_deinitialize( );
  crude_log_deinitialize( );
}

void
crude_engine_initialize_allocators_
(
  _In_ crude_engine                                       *engine
)
{ 
  crude_heap_allocator_initialize( &engine->cgltf_temporary_allocator, CRUDE_RMEGA( 16 ), "cgltf_temporary_allocator" );
  crude_heap_allocator_initialize( &engine->common_allocator, CRUDE_RMEGA( 16 ), "common_allocator" );
  crude_heap_allocator_initialize( &engine->resources_allocator, CRUDE_RMEGA( 16 ), "resources_allocator" );
  crude_stack_allocator_initialize( &engine->temporary_allocator, CRUDE_RMEGA( 16 ), "temprorary_allocator" );
  crude_stack_allocator_initialize( &engine->model_renderer_resources_manager_temporary_allocator, CRUDE_RMEGA( 64 ), "model_renderer_resources_manager_temporary_allocator" );
}

static void
crude_engine_deinitialize_allocators_
(
  _In_ crude_engine                                       *engine
)
{
  crude_heap_allocator_deinitialize( &engine->cgltf_temporary_allocator );
  crude_heap_allocator_deinitialize( &engine->common_allocator );
  crude_heap_allocator_deinitialize( &engine->resources_allocator );
  crude_stack_allocator_deinitialize( &engine->temporary_allocator );
  crude_stack_allocator_deinitialize( &engine->model_renderer_resources_manager_temporary_allocator );
}

void
crude_engine_initialize_ecs_
(
  _In_ crude_engine                                       *engine
)
{
  engine->world = crude_ecs_init();
  engine->running = true;
  engine->last_update_time = crude_time_now();

  ECS_TAG_DEFINE( engine->world, crude_entity_tag );
  
  crude_ecs_set_threads( engine->world, 1 );
}

static void
crude_engine_deinitialize_ecs_
(
  _In_ crude_engine                                       *engine
)
{
  crude_ecs_deinitalize( engine->world );
}

void
crude_engine_initialize_tash_sheduler_
(
  _In_ crude_engine                                       *engine
)
{
  crude_task_sheduler_initialize( &engine->task_sheduler, CRUDE_ENGINE_TOTAL_THREADS_COUNT );
}

void
crude_engine_deinitialize_task_sheduler_
(
  _In_ crude_engine                                       *engine
)
{
  crude_task_sheduler_deinitialize( &engine->task_sheduler );
}

void
crude_engine_initialize_imgui_
(
  _In_ crude_engine                                       *engine
)
{
  ImGuiIO                                                 *imgui_io;
  
  engine->pub_engine_should_proccess_imgui_input = true;

  IMGUI_CHECKVERSION();
  engine->imgui_context = ImGui::CreateContext();
  ImGui::SetCurrentContext( engine->imgui_context );
  ImGui::StyleColorsDark();
  imgui_io = &ImGui::GetIO();
  imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imgui_io->ConfigWindowsResizeFromEdges = true;
  imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;

  engine->imgui_font = imgui_io->Fonts->AddFontFromFileTTF( /* TODO */ game->game_font_absolute_filepath, 20.f );
  CRUDE_ASSERT( engine->imgui_font );
}

void
crude_engine_deinitialize_imgui_
(
  _In_ crude_engine                                       *engine
)
{

  ImGui::DestroyContext( engine->imgui_context );
}

void
crude_engine_initialize_debug_
(
  _In_ crude_engine                                       *engine
)
{
#if CRUDE_DEVELOP
  engine->physics_debug_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_debug_system_context );
  engine->physics_debug_system_context.resources_absolute_directory = engine->resources_absolute_directory;
  engine->physics_debug_system_context.string_bufffer = &engine->debug_strings_buffer;
  crude_physics_debug_system_import( engine->world, &engine->physics_debug_system_context );
#endif
}

void
crude_engine_deinitialize_debug_
(
  _In_ crude_engine                                       *engine
)
{
}

static void
crude_engine_initialize_audio_
(
  _In_ crude_engine                                       *engine
)
{
  engine->pub_engine_audio_volume = 1.f;

  crude_audio_device_initialize( &engine->audio_device, &engine->allocator );
  
  engine->audio_system_context = CRUDE_COMPOUNT_EMPTY( crude_audio_system_context );
  engine->audio_system_context.device = &engine->audio_device;
  crude_audio_system_import( engine->world, &engine->audio_system_context );
}

static void
crude_engine_deinitialize_audio_
(
  _In_ crude_engine                                       *engine
)
{
  crude_audio_device_deinitialize( &engine->audio_device );
}

void
crude_engine_initialize_platform_
(
  _In_ crude_engine                                       *engine,
  _In_ char const                                         *window_title,
  _In_ uint64                                              window_width,
  _In_ uint64                                              window_height
)
{
  crude_platform_creation                                  creation;

  creation = CRUDE_COMPOUNT_EMPTY( crude_platform_creation );
  creation.quit_callback;
  creation.quit_callback_ctx;
  creation.window.width = window_width;
  creation.window.height = window_height;
  creation.window.title = window_title;
  creation.window.flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  creation.window.maximized = false;
  creation.input_callback = crude_engine_input_callback_;
  creation.input_callback_ctx = engine;

  crude_platform_intialize( &engine->platform, &creation );
}

void
crude_engine_deinitialize_platform_
(
  _In_ crude_engine                                       *engine
)
{
  crude_platform_deintialize( &engine->platform );
}

void
crude_engine_initialize_graphics_
(
  _In_ crude_engine                                       *engine
)
{
  char const                                              *render_graph_file_path;
  crude_string_buffer                                      temporary_name_buffer;
  crude_gfx_model_renderer_resources_manager_creation      model_renderer_resources_manager_creation;
  crude_gfx_device_creation                                device_creation;
  crude_gfx_scene_renderer_creation                        scene_renderer_creation;
  uint32                                                   temporary_allocator_marker;
  
  crude_gfx_asynchronous_loader_manager_intiailize( &engine->asynchronous_loader_manager, &engine->task_sheduler, 1u );

  temporary_allocator_marker = crude_stack_allocator_get_marker( &engine->temporary_allocator );
  
  crude_string_buffer_initialize( &temporary_name_buffer, 1024, crude_stack_allocator_pack( &engine->temporary_allocator ) );

  device_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_device_creation );
  device_creation.sdl_window = engine->platform.sdl_window;
  device_creation.vk_application_name = "CrudeEngine";
  device_creation.vk_application_version = VK_MAKE_VERSION( 1, 0, 0 );
  device_creation.allocator_container = crude_heap_allocator_pack( &engine->common_allocator );
  device_creation.temporary_allocator = &engine->temporary_allocator;
  device_creation.queries_per_frame = 1u;
  device_creation.num_threads = engine->asynchronous_loader_manager.active_async_loaders_max_count;
  device_creation.shaders_absolute_directory = engine->shaders_absolute_directory;
  device_creation.techniques_absolute_directory = engine->techniques_absolute_directory;
  device_creation.compiled_shaders_absolute_directory = engine->compiled_shaders_absolute_directory;
  crude_gfx_device_initialize( &engine->gpu, &device_creation );
  
  crude_gfx_render_graph_builder_initialize( &engine->render_graph_builder, &engine->gpu );
  crude_gfx_render_graph_initialize( &engine->render_graph, &engine->render_graph_builder );
  
  crude_gfx_asynchronous_loader_initialize( &engine->async_loader, &engine->gpu );
  crude_gfx_asynchronous_loader_manager_add_loader( &engine->asynchronous_loader_manager, &engine->async_loader );

//#if CRUDE_DEVELOP
  render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", game->render_graph_absolute_directory, "game\\render_graph_develop.json" );
//#else
//  render_graph_file_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", game->render_graph_absolute_directory, "game\\render_graph_production.json" );
//#endif
  crude_gfx_render_graph_parse_from_file( &game->render_graph, render_graph_file_path, &game->temporary_allocator );
  crude_gfx_render_graph_compile( &game->render_graph, &game->temporary_allocator );
  
  if ( game->gpu.mesh_shaders_extension_present )
  {
    crude_gfx_technique_load_from_file( "deferred_meshlet.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  }
  else
  {
    crude_gfx_technique_load_from_file( "deferred_classic.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  }
  //crude_gfx_technique_load_from_file( "pointshadow_meshlet.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "compute.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "debug.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "fullscreen.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "imgui.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  crude_gfx_technique_load_from_file( "game/fullscreen.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
  
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_renderer_technique_load_from_file( "ray_tracing_solid.json", &game->gpu, &game->render_graph, &game->temporary_allocator );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
  
  model_renderer_resources_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_manager_creation );
  model_renderer_resources_manager_creation.allocator = &game->allocator;
  model_renderer_resources_manager_creation.async_loader = &game->async_loader;
  model_renderer_resources_manager_creation.cgltf_temporary_allocator = &game->cgltf_temporary_allocator;
  model_renderer_resources_manager_creation.temporary_allocator = &game->model_renderer_resources_manager_temporary_allocator;
  model_renderer_resources_manager_creation.world = game->engine->world;
  crude_gfx_model_renderer_resources_manager_intialize( &game->model_renderer_resources_manager, &model_renderer_resources_manager_creation );

  scene_renderer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_creation );
  scene_renderer_creation.async_loader = &game->async_loader;
  scene_renderer_creation.allocator = &game->allocator;
  scene_renderer_creation.temporary_allocator = &game->temporary_allocator;
  scene_renderer_creation.task_scheduler = game->engine->asynchronous_loader_manager.task_sheduler;
  scene_renderer_creation.model_renderer_resources_manager = &game->model_renderer_resources_manager;
  scene_renderer_creation.imgui_pass_enalbed = true;
  scene_renderer_creation.imgui_context = game->imgui_context;
  crude_gfx_scene_renderer_initialize( &game->scene_renderer, &scene_renderer_creation );

#if CRUDE_DEVELOP
  game->scene_renderer.options.hide_collision = true;
  game->scene_renderer.options.hide_debug_gltf = true;
#endif

  game->scene_renderer.options.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 1, 1, 1 } );
  game->scene_renderer.options.ambient_intensity = 1.5f;
  game->scene_renderer.options.background_intensity = 0.f;
  game->scene_renderer.options.hdr_pre_tonemapping_texture_name = "game_hdr_pre_tonemapping";

  game->graphics_time = 0.f;
  
  game_setup_custom_postload_model_resources_( game );
  crude_gfx_scene_renderer_rebuild_light_gpu_buffers( &game->scene_renderer );

  crude_gfx_scene_renderer_initialize_pases( &game->scene_renderer );
  crude_gfx_scene_renderer_register_passes( &game->scene_renderer, &game->render_graph );
  crude_gfx_render_graph_builder_register_render_pass( game->render_graph.builder, "game_postprocessing_pass", crude_gfx_game_postprocessing_pass_pack( &game->game_postprocessing_pass ) );

  crude_stack_allocator_free_marker( &game->temporary_allocator, temporary_allocator_marker );
}


void
crude_engine_deinitialize_graphics_
(
  _In_ crude_engine                                       *engine
)
{
  crude_gfx_asynchronous_loader_manager_remove_loader( &game->engine->asynchronous_loader_manager, &game->async_loader );
  vkDeviceWaitIdle( game->gpu.vk_device );
  crude_gfx_scene_renderer_deinitialize_passes( &game->scene_renderer );
  crude_gfx_game_postprocessing_pass_deinitialize( &game->game_postprocessing_pass );
  crude_gfx_scene_renderer_deinitialize( &game->scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &game->async_loader );
  crude_gfx_model_renderer_resources_manager_deintialize( &game->model_renderer_resources_manager );
  crude_gfx_render_graph_builder_deinitialize( &game->render_graph_builder );
  crude_gfx_render_graph_deinitialize( &game->render_graph );
  crude_gfx_device_deinitialize( &game->gpu );
  crude_gfx_asynchronous_loader_manager_deintiailize( &engine->asynchronous_loader_manager );
}

void
crude_engine_initialize_collisions_
(
  _In_ crude_engine                                       *engine
)
{
  crude_collisions_resources_manager_deinitialize( &engine->collision_resources_manager );
}

void
crude_engine_deinitialize_collisions_
(
  _In_ crude_engine                                       *engine
)
{
  crude_collisions_resources_manager_initialize( &engine->collision_resources_manager, &engine->common_allocator, &engine->cgltf_temporary_allocator );
}

void
crude_engine_initialize_physics_
(
  _In_ crude_engine                                       *engine
)
{
  crude_physics_resources_manager_creation                 physics_resources_manager_creation;
  crude_physics_creation                                   physics_creation;

  physics_resources_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_physics_resources_manager_creation );
  physics_resources_manager_creation.allocator = &engine->common_allocator;
  crude_physics_resources_manager_initialize( &engine->physics_resources_manager, &physics_resources_manager_creation );

  physics_creation = CRUDE_COMPOUNT_EMPTY( crude_physics_creation );
  physics_creation.collision_manager = &engine->collision_resources_manager;
  physics_creation.manager = &engine->physics_resources_manager;
  physics_creation.world = engine->world;
  crude_physics_initialize( &engine->physics, &physics_creation );

  engine->physics_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_system_context );
  engine->physics_system_context.physics = &engine->physics;
  crude_physics_system_import( engine->world, &engine->physics_system_context );
}

void
crude_engine_deinitialize_physics_
(
  _In_ crude_engine                                       *engine
)
{
}

void
crude_engine_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
)
{
  crude_engine *engine = CRUDE_CAST( crude_engine*, ctx );
  ImGui::SetCurrentContext( engine->imgui_context );
  
  if ( engine->pub_engine_should_proccess_imgui_input )
  {
    ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
  }
}

void
crude_engine_initialize_scene_
(
  _In_ crude_engine                                       *engine
)
{
  crude_node_manager_creation                              node_manager_creation;
  node_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_node_manager_creation );
  node_manager_creation.world = engine->world;
  node_manager_creation.resources_absolute_directory = engine->resources_absolute_directory;
  node_manager_creation.temporary_allocator = &engine->temporary_allocator;
  node_manager_creation.additional_parse_all_components_to_json_func = crude_engine_parse_all_components_to_json_;
  node_manager_creation.additional_parse_json_to_component_func = crude_engine_parse_json_to_component_;
  node_manager_creation.physics_resources_manager = &engine->physics_resources_manager;
  node_manager_creation.collisions_resources_manager = &engine->collision_resources_manager;
  node_manager_creation.allocator = &engine->common_allocator;
  crude_node_manager_initialize( &engine->node_manager, &node_manager_creation );
}

void
crude_engine_deinitialize_scene_
(
  _In_ crude_engine                                       *engine
)
{
  crude_node_manager_deinitialize( &engine->node_manager );
}

bool
crude_engine_parse_json_to_component_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON const                                        *component_json,
  _In_ char const                                         *component_name,
  _In_ crude_node_manager                                 *manager
)
{
  return true;
}

void
crude_engine_parse_all_components_to_json_
( 
  _In_ crude_entity                                        node, 
  _In_ cJSON                                               *node_components_json,
  _In_ crude_node_manager                                 *manager
)
{
}

void
crude_engine_pinned_task_platform_loop_
(
  _In_ void                                               *ctx
)
{
  crude_engine *engine = CRUDE_REINTERPRET_CAST( crude_engine*, ctx );
  crude_platform_update( &engine->platform );
}

void
crude_engine_pinned_task_graphics_loop_
(
  _In_ void                                               *ctx
)
{
  crude_engine                                            *engine;
  crude_gfx_texture                                       *final_render_texture;
  float32                                                  last_graphics_update_delta;
  bool                                                     new_buffers_recrteated_or_model_initialized;
  
  CRUDE_PROFILER_ZONE_NAME( "crude_engine_pinned_task_graphics_loop_" );
  
  engine = CRUDE_REINTERPRET_CAST( crude_engine*, ctx );
  final_render_texture = crude_gfx_access_texture( &engine->gpu, crude_gfx_render_graph_builder_access_resource_by_name( engine->scene_renderer.render_graph->builder, CRUDE_GRAPHICS_PRESENT_TEXTURE_NAME )->resource_info.texture.handle );

  last_graphics_update_delta = crude_time_delta_seconds( engine->last_graphics_update_time, crude_time_now( ) );

  if ( !crude_entity_valid( engine->graphics_focused_camera_node ) )
  {
    goto cleanup;
  }

  if ( last_graphics_update_delta < 1.f / engine->graphics_framerate )
  {
    goto cleanup;
  }

  engine->graphics_absolute_time += last_graphics_update_delta;
  
  engine->last_graphics_update_time = crude_time_now( );

  engine->scene_renderer.options.camera_node = engine->graphics_focused_camera_node;
  engine->scene_renderer.options.absolute_time = engine->graphics_absolute_time;

  crude_gfx_new_frame( &engine->gpu );
  
  mtx_lock( &engine->nodes_mutex );
  new_buffers_recrteated_or_model_initialized = crude_gfx_scene_renderer_update_instances_from_node( &engine->scene_renderer, engine->graphics_main_node );
  mtx_unlock( &engine->nodes_mutex );
  
  if ( new_buffers_recrteated_or_model_initialized )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Model being loaded during scene rendering!" );
    crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &engine->model_renderer_resources_manager );
  }
 
  ImGui::SetCurrentContext( engine->imgui_context );
  ImGui_ImplSDL3_NewFrame( );
  ImGui::NewFrame( );
#if CRUDE_DEVELOP
  //crude_devmenu_draw( &game->devmenu );
#endif
  
  if ( engine->gpu.swapchain_resized_last_frame )
  {
    crude_gfx_scene_renderer_on_resize( &engine->scene_renderer );
    crude_gfx_render_graph_on_resize( &engine->render_graph, engine->gpu.vk_swapchain_width, engine->gpu.vk_swapchain_height );
  }

  crude_gfx_scene_renderer_submit_draw_task( &engine->scene_renderer, false );

  crude_gfx_present( &engine->gpu, final_render_texture );

cleanup:
  CRUDE_PROFILER_ZONE_END;
}