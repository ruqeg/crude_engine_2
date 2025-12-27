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

  crude_graphics_thread_manager_stop( &engine->___graphics_thread_manager );
  crude_scene_thread_manager_stop( &engine->___scene_thread_manager );
  
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
  
  crude_devmenu_update( &engine->devmenu );
  crude_engine_commands_manager_update( &engine->commands_manager );
  crude_platform_update( &engine->platform );
  crude_input_thread_data_affect_by_input( &engine->__input_thread_data, &engine->platform.input );

cleanup:
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
  crude_scene_thread_manager_initialize( &engine->___scene_thread_manager, &engine->task_sheduler, &engine->__input_thread_data );
}

static void
crude_engine_deinitialize_ecs_
(
  _In_ crude_engine                                       *engine
)
{
  crude_scene_thread_manager_deinitialize( &engine->___scene_thread_manager );
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

  mtx_init( &engine->imgui_mutex, mtx_plain );
  //engine->imgui_font = imgui_io->Fonts->AddFontFromFileTTF( /* TODO */ game->game_font_absolute_filepath, 20.f );
  //CRUDE_ASSERT( engine->imgui_font );
}

void
crude_engine_deinitialize_imgui_
(
  _In_ crude_engine                                       *engine
)
{
  mtx_destroy( &engine->imgui_mutex );
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

  crude_ecs *world = crude_scene_thread_manager_lock_world( &engine->___scene_thread_manager );
  crude_audio_system_import( world, &engine->audio_system_context );
  crude_scene_thread_manager_unlock_world( &engine->___scene_thread_manager );
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
  crude_input_thread_data_initialize( &engine->__input_thread_data );
}

void
crude_engine_deinitialize_platform_
(
  _In_ crude_engine                                       *engine
)
{
  crude_input_thread_data_deinitialize( &engine->__input_thread_data );
  crude_platform_deintialize( &engine->platform );
}

void
crude_engine_initialize_graphics_
(
  _In_ crude_engine                                       *engine
)
{
  crude_gfx_asynchronous_loader_manager_intiailize( &engine->___asynchronous_loader_manager, &engine->task_sheduler, 1u );
  crude_graphics_thread_manager_initialize( &engine->___graphics_thread_manager, &engine->environment, engine->platform.sdl_window, &engine->task_sheduler, &engine->___asynchronous_loader_manager, &engine->___scene_thread_manager, &engine->imgui_mutex, engine->imgui_context, &engine->cgltf_temporary_allocator, &engine->model_renderer_resources_manager_temporary_allocator, &engine->common_allocator, &engine->temporary_allocator, &engine->devmenu );
}


void
crude_engine_deinitialize_graphics_
(
  _In_ crude_engine                                       *engine
)
{
  crude_gfx_asynchronous_loader_manager_deintiailize( &engine->___asynchronous_loader_manager );
  crude_graphics_thread_manager_deinitialize( &engine->___graphics_thread_manager );
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
  crude_ecs                                               *world;
  crude_physics_resources_manager_creation                 physics_resources_manager_creation;
  crude_physics_creation                                   physics_creation;

  physics_resources_manager_creation = CRUDE_COMPOUNT_EMPTY( crude_physics_resources_manager_creation );
  physics_resources_manager_creation.allocator = &engine->common_allocator;
  crude_physics_resources_manager_initialize( &engine->physics_resources_manager, &physics_resources_manager_creation );

  physics_creation = CRUDE_COMPOUNT_EMPTY( crude_physics_creation );
  physics_creation.collision_manager = &engine->collision_resources_manager;
  physics_creation.manager = &engine->physics_resources_manager;
  
  world = crude_scene_thread_manager_lock_world( &engine->___scene_thread_manager );
  crude_physics_initialize( &engine->physics, &physics_creation, world );

  engine->physics_system_context = CRUDE_COMPOUNT_EMPTY( crude_physics_system_context );
  engine->physics_system_context.physics = &engine->physics;
  
  crude_physics_system_import( world, &engine->physics_system_context );
  crude_scene_thread_manager_unlock_world( &engine->___scene_thread_manager );
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
  
  mtx_lock( &engine->imgui_mutex );
  ImGui::SetCurrentContext( engine->imgui_context );

  if ( engine->pub_engine_should_proccess_imgui_input )
  {
    ImGui_ImplSDL3_ProcessEvent( CRUDE_CAST( SDL_Event*, sdl_event ) );
  }
  mtx_unlock( &engine->imgui_mutex );
  
  crude_devmenu_handle_input( &engine->devmenu, &engine->platform.input, engine->___scene_thread_manager.world, &engine->___graphics_thread_manager, &engine->___scene_thread_manager );
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
  crude_ecs *world = crude_scene_thread_manager_lock_world( &engine->___scene_thread_manager );
  crude_devmenu_initialize( &engine->devmenu, engine, world, &engine->___graphics_thread_manager, &engine->___scene_thread_manager );
  crude_scene_thread_manager_unlock_world( &engine->___scene_thread_manager );
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