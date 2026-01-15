#include <engine/core/profiler.h>
#include <engine/core/time.h>
#include <engine/core/array.h>
#include <engine/core/log.h>
#include <engine/core/ecs.h>
#include <engine/core/file.h>
#include <engine/platform/platform.h>
#include <engine/physics/physics_debug_ecs.h>
#include <engine/graphics/gpu_resources_loader.h>
#include <engine/engine/environment.h>

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
crude_engine_initialize_devmenu_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_deinitialize_devmenu_
(
  _In_ crude_engine                                       *engine
);

static void
crude_engine_input_callback_
(
  _In_ void                                               *ctx,
  _In_ void                                               *sdl_event
);

void
crude_engine_quit_callback_
(
  _In_ void                                               *ctx
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

bool
crude_engine_graphics_main_thread_loop_
(
  _In_ crude_engine                                       *engine
);

void
crude_engine_graphics_task_set_thread_loop_
(
  _In_ uint32_t                                            start_,
  _In_ uint32_t                                            end_,
  _In_ uint32_t                                            threadnum_,
  _In_ void                                               *ctx
);

void
crude_engine_initialize
(
  _In_ crude_engine                                       *engine,
  _In_ char const                                         *working_directory
)
{
  char                                                     environment_absolute_filepath[ 4096 ];

  crude_snprintf( environment_absolute_filepath, CRUDE_COUNTOF( environment_absolute_filepath ), "%s\\%s", working_directory, CRUDE_ENGINE_ENVIRONMENT_INITIAL );

  *engine = CRUDE_COMPOUNT_EMPTY( crude_engine );
  crude_engine_initialize_services_( );
  crude_engine_initialize_allocators_( engine );
  crude_environment_initialize( &engine->environment, environment_absolute_filepath, working_directory, &engine->common_allocator, &engine->temporary_allocator );
  crude_engine_initialize_tash_sheduler_( engine );
  crude_engine_initialize_platform_( engine, engine->environment.window.initial_title, engine->environment.window.initial_width, engine->environment.window.initial_height );
  crude_engine_initialize_ecs_( engine );
  crude_engine_initialize_imgui_( engine );
  crude_engine_initialize_debug_( engine );
  crude_engine_initialize_audio_( engine );
  crude_engine_initialize_graphics_( engine );
  crude_engine_initialize_collisions_( engine );
  crude_engine_initialize_physics_( engine );
  crude_engine_initialize_scene_( engine );
  crude_engine_commands_manager_initialize( &engine->commands_manager, engine, &engine->common_allocator );
  crude_engine_initialize_devmenu_( engine );

  engine->running = true;
}

void
crude_engine_deinitialize
(
  _In_ crude_engine                                       *engine
)
{
  engine->running = false;

  crude_engine_deinitialize_devmenu_( engine );
  crude_engine_commands_manager_deinitialize( &engine->commands_manager );
  crude_engine_deinitialize_scene_( engine );
  crude_engine_deinitialize_physics_( engine );
  crude_engine_deinitialize_collisions_( engine );
  crude_engine_deinitialize_graphics_( engine );
  crude_engine_deinitialize_audio_( engine );
  crude_engine_deinitialize_debug_( engine );
  crude_engine_deinitialize_imgui_( engine );
  crude_engine_deinitialize_ecs_( engine );
  crude_engine_deinitialize_platform_( engine );
  crude_engine_deinitialize_task_sheduler_( engine );
  crude_environment_deinitialize( &engine->environment );
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
  
  crude_platform_update( &engine->platform );
  crude_devmenu_update( &engine->devmenu );
  
  current_time = crude_time_now( );
  delta_time = crude_time_delta_seconds( engine->last_update_time, current_time );
  crude_ecs_progress( engine->world, delta_time );
  engine->last_update_time = current_time;

  {
    CRUDE_PROFILER_ZONE_NAME( "crude_task_sheduler_wait_task_set ( engine->graphics_task_set_handle )" );
    crude_task_sheduler_wait_task_set( &engine->task_sheduler, engine->graphics_task_set_handle );
    CRUDE_PROFILER_ZONE_END;
  }
  crude_engine_commands_manager_update( &engine->commands_manager );
  
  if ( crude_engine_graphics_main_thread_loop_( engine )  )
  {
    crude_task_sheduler_start_task_set( &engine->task_sheduler, engine->graphics_task_set_handle );
  }

  CRUDE_PROFILER_ZONE_END;
  return engine->running;
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
  crude_heap_allocator_initialize( &engine->common_allocator, CRUDE_RMEGA( 16 ), "common_allocator" );
  crude_heap_allocator_initialize( &engine->resources_allocator, CRUDE_RMEGA( 16 ), "resources_allocator" );
  crude_stack_allocator_initialize( &engine->temporary_allocator, CRUDE_RMEGA( 16 ), "temprorary_allocator" );
  crude_heap_allocator_initialize( &engine->cgltf_temporary_allocator, CRUDE_RMEGA( 16 ), "cgltf_temporary_allocator" );
  crude_stack_allocator_initialize( &engine->model_renderer_resources_manager_temporary_allocator, CRUDE_RMEGA( 64 ), "model_renderer_resources_manager_temporary_allocator" );
}

static void
crude_engine_deinitialize_allocators_
(
  _In_ crude_engine                                       *engine
)
{
  crude_heap_allocator_deinitialize( &engine->common_allocator );
  crude_heap_allocator_deinitialize( &engine->resources_allocator );
  crude_stack_allocator_deinitialize( &engine->temporary_allocator );
}

void
crude_engine_initialize_ecs_
(
  _In_ crude_engine                                       *engine
)
{
  engine->last_update_time = crude_time_now();
  engine->player_controller_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  engine->camera_node = CRUDE_COMPOUNT_EMPTY( crude_entity );
  engine->main_node = CRUDE_COMPOUNT_EMPTY( crude_entity );

  engine->world = crude_ecs_create( );
  
  CRUDE_ECS_TAG_DEFINE( engine->world, crude_entity_tag );
  
  crude_ecs_set_threads( engine->world, 1 );
}

static void
crude_engine_deinitialize_ecs_
(
  _In_ crude_engine                                       *engine
)
{
  crude_ecs_destroy( engine->world );
}

void
crude_engine_initialize_tash_sheduler_
(
  _In_ crude_engine                                       *engine
)
{
  crude_task_sheduler_initialize( &engine->task_sheduler );
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

  IMGUI_CHECKVERSION();
  engine->imgui_context = ImGui::CreateContext();
  ImGui::SetCurrentContext( engine->imgui_context );
  ImGui::StyleColorsDark();
  imgui_io = &ImGui::GetIO();
  imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  //imgui_io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  imgui_io->ConfigWindowsResizeFromEdges = true;
  imgui_io->ConfigWindowsMoveFromTitleBarOnly = true;
  
  ImGui_ImplSDL3_InitForVulkan( engine->platform.sdl_window );

  engine->pub_engine_should_proccess_imgui_input = true;

  //engine->imgui_font = imgui_io->Fonts->AddFontFromFileTTF( /* TODO */ game->game_font_absolute_filepath, 20.f );
  //CRUDE_ASSERT( engine->imgui_font );
}

void
crude_engine_deinitialize_imgui_
(
  _In_ crude_engine                                       *engine
)
{
  engine->pub_engine_should_proccess_imgui_input = false;
  ImGui_ImplSDL3_Shutdown( );
  ImGui::DestroyContext( engine->imgui_context );
}

void
crude_engine_initialize_debug_
(
  _In_ crude_engine                                       *engine
)
{
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

  crude_audio_device_initialize( &engine->audio_device, &engine->common_allocator );
  
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
  creation.quit_callback = crude_engine_quit_callback_;
  creation.quit_callback_ctx = engine;
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
  char                                                     render_graph_file_path[ 4096 ];
  crude_gfx_device_creation                                device_creation;
  crude_gfx_scene_renderer_creation                        scene_renderer_creation;
  crude_gfx_model_renderer_resources_manager_creation      model_renderer_resources_manager_creation;
  uint64                                                   temporary_allocator_marker;

  crude_gfx_asynchronous_loader_manager_intiailize( &engine->asynchronous_loader_manager, &engine->task_sheduler, 1u );

  temporary_allocator_marker = crude_stack_allocator_get_marker( &engine->temporary_allocator );
  
  device_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_device_creation );
  device_creation.sdl_window = engine->platform.sdl_window;
  device_creation.vk_application_name = "CrudeEngine";
  device_creation.vk_application_version = VK_MAKE_VERSION( 1, 0, 0 );
  device_creation.allocator_container = crude_heap_allocator_pack( &engine->common_allocator );
  device_creation.temporary_allocator = &engine->temporary_allocator;
  device_creation.queries_per_frame = 1u;
  device_creation.num_threads = 3;
  device_creation.shaders_absolute_directory = engine->environment.directories.shaders_absolute_directory;
  device_creation.techniques_absolute_directory = engine->environment.directories.techniques_absolute_directory;
  device_creation.compiled_shaders_absolute_directory = engine->environment.directories.compiled_shaders_absolute_directory;
  crude_gfx_device_initialize( &engine->gpu, &device_creation );
  
  crude_gfx_render_graph_builder_initialize( &engine->render_graph_builder, &engine->gpu );
  crude_gfx_render_graph_initialize( &engine->render_graph, &engine->render_graph_builder );
  
  crude_gfx_asynchronous_loader_initialize( &engine->async_loader, &engine->gpu );
  crude_gfx_asynchronous_loader_manager_add_loader( &engine->asynchronous_loader_manager, &engine->async_loader );

  crude_snprintf( render_graph_file_path, sizeof( render_graph_file_path ), "%s%s", engine->environment.directories.render_graph_absolute_directory, "render_graph.json" );
  crude_gfx_render_graph_parse_from_file( &engine->render_graph, render_graph_file_path, &engine->temporary_allocator );
  crude_gfx_render_graph_compile( &engine->render_graph, &engine->temporary_allocator );
  
  if ( engine->gpu.mesh_shaders_extension_present )
  {
    crude_gfx_technique_load_from_file( "deferred_meshlet.crude_techniques", &engine->gpu, &engine->render_graph, &engine->temporary_allocator );
  }
  else
  {
    crude_gfx_technique_load_from_file( "deferred_classic.crude_techniques", &engine->gpu, &engine->render_graph, &engine->temporary_allocator );
  }
  crude_gfx_technique_load_from_file( "compute.crude_techniques", &engine->gpu, &engine->render_graph, &engine->temporary_allocator );
  crude_gfx_technique_load_from_file( "debug.crude_techniques", &engine->gpu, &engine->render_graph, &engine->temporary_allocator );
  crude_gfx_technique_load_from_file( "fullscreen.crude_techniques", &engine->gpu, &engine->render_graph, &engine->temporary_allocator );
  crude_gfx_technique_load_from_file( "imgui.crude_techniques", &engine->gpu, &engine->render_graph, &engine->temporary_allocator );
  
#if CRUDE_GRAPHICS_RAY_TRACING_ENABLED
  crude_gfx_renderer_technique_load_from_file( "ray_tracing_solid.crude_techniques", &game->gpu, &game->render_graph, &&engine->temporary_allocator );
#endif /* CRUDE_GRAPHICS_RAY_TRACING_ENABLED */
  
  model_renderer_resources_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_model_renderer_resources_manager_creation );
  model_renderer_resources_manager_creation.allocator = &engine->common_allocator;
  model_renderer_resources_manager_creation.async_loader = &engine->async_loader;
  model_renderer_resources_manager_creation.cgltf_temporary_allocator = &engine->cgltf_temporary_allocator;
  model_renderer_resources_manager_creation.temporary_allocator = &engine->model_renderer_resources_manager_temporary_allocator;
  crude_gfx_model_renderer_resources_manager_intialize( &engine->model_renderer_resources_manager, &model_renderer_resources_manager_creation );

  scene_renderer_creation = CRUDE_COMPOUNT_EMPTY( crude_gfx_scene_renderer_creation );
  scene_renderer_creation.async_loader = &engine->async_loader;
  scene_renderer_creation.allocator = &engine->common_allocator;
  scene_renderer_creation.temporary_allocator = &engine->temporary_allocator;
  scene_renderer_creation.model_renderer_resources_manager = &engine->model_renderer_resources_manager;
  scene_renderer_creation.imgui_pass_enalbed = true;
  scene_renderer_creation.imgui_context = engine->imgui_context;
  crude_gfx_scene_renderer_initialize( &engine->scene_renderer, &scene_renderer_creation );

#if CRUDE_DEVELOP
  engine->scene_renderer.options.debug.hide_collision = true;
  engine->scene_renderer.options.debug.hide_debug_gltf = true;
#endif

  engine->scene_renderer.options.scene.ambient_color = CRUDE_COMPOUNT( XMFLOAT3, { 1, 1, 1 } );
  //engine->scene_renderer.options.scene.ambient_intensity = 0.f;
  engine->scene_renderer.options.scene.background_intensity = 0.f;

  engine->graphics_absolute_time = 0.f;
  engine->framerate = 120;

  crude_gfx_scene_renderer_rebuild_light_gpu_buffers( &engine->scene_renderer );

  crude_gfx_scene_renderer_register_passes( &engine->scene_renderer, &engine->render_graph );

  engine->graphics_task_set_handle = crude_task_sheduler_create_task_set( &engine->task_sheduler, crude_engine_graphics_task_set_thread_loop_, engine );
}


void
crude_engine_deinitialize_graphics_
(
  _In_ crude_engine                                       *engine
)
{
  vkDeviceWaitIdle( engine->gpu.vk_device );
  crude_task_sheduler_wait_task_set( &engine->task_sheduler, engine->graphics_task_set_handle );
  crude_task_sheduler_destroy_task_set( &engine->task_sheduler, engine->graphics_task_set_handle );
  crude_gfx_asynchronous_loader_manager_remove_loader( &engine->asynchronous_loader_manager, &engine->async_loader );
  crude_gfx_scene_renderer_deinitialize( &engine->scene_renderer );
  crude_gfx_asynchronous_loader_deinitialize( &engine->async_loader );
  crude_gfx_model_renderer_resources_manager_deintialize( &engine->model_renderer_resources_manager );
  crude_gfx_render_graph_builder_deinitialize( &engine->render_graph_builder );
  crude_gfx_render_graph_deinitialize( &engine->render_graph );
  crude_gfx_device_deinitialize( &engine->gpu );
  crude_gfx_asynchronous_loader_manager_deintiailize( &engine->asynchronous_loader_manager );
}

void
crude_engine_initialize_collisions_
(
  _In_ crude_engine                                       *engine
)
{
  crude_collisions_resources_manager_initialize( &engine->collision_resources_manager, &engine->common_allocator, &engine->cgltf_temporary_allocator );
}

void
crude_engine_deinitialize_collisions_
(
  _In_ crude_engine                                       *engine
)
{
  crude_collisions_resources_manager_deinitialize( &engine->collision_resources_manager );
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
  
  crude_physics_initialize( &engine->physics, &physics_creation, engine->world );

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
  crude_physics_deinitialize( &engine->physics );
  crude_physics_resources_manager_deinitialize( &engine->physics_resources_manager );
}

void
crude_engine_quit_callback_
(
  _In_ void                                               *ctx
)
{
  crude_engine *engine = CRUDE_CAST( crude_engine*, ctx );
  engine->running = false;
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
  
  crude_devmenu_handle_input( &engine->devmenu );
}

void
crude_engine_initialize_scene_
(
  _In_ crude_engine                                       *engine
)
{
  crude_node_manager_creation                              node_manager_creation;
  node_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_node_manager_creation );
  node_manager_creation.resources_absolute_directory = engine->environment.directories.resources_absolute_directory;
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

void
crude_engine_initialize_devmenu_
(
  _In_ crude_engine                                       *engine
)
{
  crude_devmenu_initialize( &engine->devmenu, engine );
}

void
crude_engine_deinitialize_devmenu_
(
  _In_ crude_engine                                       *engine
)
{
  crude_devmenu_deinitialize( &engine->devmenu );
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

bool
crude_engine_graphics_main_thread_loop_
(
  _In_ crude_engine                                       *engine
)
{
  float32                                                  last_graphics_update_delta;
  bool                                                     new_buffers_recrteated_or_model_initialized;

  CRUDE_PROFILER_ZONE_NAME( "crude_engine_graphics_main_thread_loop_" );
  
  last_graphics_update_delta = crude_time_delta_seconds( engine->last_graphics_update_time, crude_time_now( ) );
    
  if ( last_graphics_update_delta < 1.f / engine->framerate )
  {
    CRUDE_PROFILER_ZONE_END;
    return false;
  }
  
  if ( !crude_entity_valid( engine->world, engine->camera_node ) )
  {
    CRUDE_PROFILER_ZONE_END;
    return false;
  }

  ImGui::SetCurrentContext( engine->imgui_context );
  ImGuizmo::SetImGuiContext( engine->imgui_context );

  engine->graphics_absolute_time += last_graphics_update_delta;
  engine->last_graphics_update_time = crude_time_now( );
  engine->scene_renderer.options.absolute_time = engine->graphics_absolute_time;

  crude_gfx_new_frame( &engine->gpu );
  
  new_buffers_recrteated_or_model_initialized = crude_gfx_scene_renderer_update_instances_from_node( &engine->scene_renderer, engine->world, engine->main_node );
  engine->scene_renderer.options.scene.camera = *CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( engine->world, engine->camera_node, crude_camera );
  XMStoreFloat4x4( &engine->scene_renderer.options.scene.camera_view_to_world, crude_transform_node_to_world( engine->world, engine->camera_node, CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( engine->world, engine->camera_node, crude_transform ) ) );

  if ( new_buffers_recrteated_or_model_initialized )
  {
    CRUDE_LOG_ERROR( CRUDE_CHANNEL_GRAPHICS, "Model being loaded during scene rendering!" );
    crude_gfx_model_renderer_resources_manager_wait_till_uploaded( &engine->model_renderer_resources_manager );
  }
 
  ImGui_ImplSDL3_NewFrame( );
  ImGui::NewFrame( );
  ImGuizmo::SetOrthographic( false );
  ImGuizmo::BeginFrame();
  //ImGui::DockSpaceOverViewport( 0u, ImGui::GetMainViewport( ) );

  crude_devmenu_draw( &engine->devmenu );
  if ( engine->imgui_draw_custom_fn )
  {
    engine->imgui_draw_custom_fn( engine->imgui_draw_custom_ctx );
  }

  CRUDE_PROFILER_ZONE_END;
  return true;
}

void
crude_engine_graphics_task_set_thread_loop_
(
  _In_ uint32_t                                            start_,
  _In_ uint32_t                                            end_,
  _In_ uint32_t                                            threadnum_,
  _In_ void                                               *ctx
) 
{
  crude_engine                                            *engine;
  crude_gfx_texture                                       *final_render_texture;

  engine = CRUDE_REINTERPRET_CAST( crude_engine*, ctx );;
  
  CRUDE_PROFILER_ZONE_NAME( "crude_engine_graphics_task_set_thread_loop_" );
  
  final_render_texture = crude_gfx_access_texture( &engine->gpu, crude_gfx_render_graph_builder_access_resource_by_name( engine->scene_renderer.render_graph->builder, CRUDE_GRAPHICS_PRESENT_TEXTURE_NAME )->resource_info.texture.handle );

  if ( engine->gpu.swapchain_resized_last_frame )
  {
    crude_gfx_scene_renderer_on_resize( &engine->scene_renderer );
    crude_gfx_render_graph_on_resize( &engine->render_graph, engine->gpu.renderer_size.x, engine->gpu.renderer_size.y );
  }

  crude_gfx_scene_renderer_submit_draw_task( &engine->scene_renderer, false );

  crude_gfx_present( &engine->gpu, final_render_texture );

  CRUDE_PROFILER_ZONE_END;
}