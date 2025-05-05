#include <flecs.h>

#include <engine.h>
#include <platform/platform_system.h>
#include <platform/platform_components.h>
#include <graphics/graphics_system.h>
#include <graphics/graphics_components.h>
#include <scene/free_camera_system.h>

#include <sandbox.h>

void
crude_sandbox_initialize
(
  _In_ crude_sandbox                                      *sandbox,
  _In_ crude_engine                                       *engine
)
{
  sandbox->engine = engine;

  crude_heap_allocator_initialize( &sandbox->graphics_allocator, CRUDE_RMEGA( 32 ), "RendererAllocator" );
  crude_stack_allocator_initialize( &sandbox->temporary_allocator, CRUDE_RMEGA( 32 ), "TemproraryAllocator" );
  
  ECS_IMPORT( sandbox->engine->world, crude_platform_system );
  ECS_IMPORT( sandbox->engine->world, crude_graphics_system );
  ECS_IMPORT( sandbox->engine->world, crude_free_camera_system );
  
  ecs_entity_t scene = ecs_entity( sandbox->engine->world, { .name = "scene1" } );
  ecs_set( sandbox->engine->world, scene, crude_window, { 
    .width     = 800,
    .height    = 600,
    .maximized = false });
  ecs_set( sandbox->engine->world, scene, crude_input, { 0 } );
  ecs_set( sandbox->engine->world, scene, crude_graphics_creation, {
    .task_sheduler       = sandbox->engine->task_sheduler,
    .allocator_container = crude_heap_allocator_pack( &sandbox->graphics_allocator ),
    .allocator_container = crude_heap_allocator_pack( &sandbox->graphics_allocator ),
    .temporary_allocator = &sandbox->temporary_allocator,
  } );

  //crude_allocator_container temporary_allocator_container = crude_stack_allocator_pack( render_create[ i ].temporary_allocator );
  //char working_directory[ 512 ];
  //crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
  //crude_string_buffer temporary_name_buffer;
  //crude_string_buffer_initialize( &temporary_name_buffer, 1024, temporary_allocator_container );
  //
  //char const *frame_graph_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\graph.json" );
  //crude_gfx_render_graph_parse_from_file( renderer->render_graph, frame_graph_path, render_create[ i ].temporary_allocator );
  //
  //
  //char const *full_screen_pipeline_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\shaders.json" );
  //
  //char const *gltf_path = crude_string_buffer_append_use_f( &temporary_name_buffer, "%s%s", working_directory, "\\..\\..\\resources\\glTF-Sample-Models\\2.0\\Sponza\\glTF\\Sponza.gltf" );
  //
  //crude_gfx_buffer_creation ubo_creation = {
  //  .type_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
  //  .usage = CRUDE_GFX_RESOURCE_USAGE_TYPE_DYNAMIC ,
  //  .size = sizeof( crude_gfx_shader_frame_constants ),
  //  .name = "ubo",
  //};
  //renderer->gpu->frame_buffer = crude_gfx_create_buffer( renderer->gpu, &ubo_creation );
  //
  //renderer->scene = CRUDE_ALLOCATE( render_create[ i ].allocator, sizeof( crude_gltf_scene ) );
  //
  //crude_gltf_scene_creation gltf_creation = {
  //  .renderer = renderer->renderer,
  //  .path = gltf_path,
  //  .async_loader = renderer->async_loader,
  //  .allocator = render_create[ i ].allocator,
  //  .temprorary_stack_allocator = render_create[ i ].temporary_allocator
  //};
  //crude_gltf_scene_load_from_file( renderer->scene, &gltf_creation );
  //
  //crude_register_render_passes( renderer->scene, renderer->render_graph );
  //
  //renderer[ i ].camera = crude_entity_create_empty( it->world, "camera1" );
  //CRUDE_ENTITY_SET_COMPONENT( renderer[ i ].camera, crude_camera, {
  //  .fov_radians = CRUDE_CPI4,
  //  .near_z = 0.01,
  //  .far_z = 1000,
  //  .aspect_ratio = 1.0 } );
  //CRUDE_ENTITY_SET_COMPONENT( renderer[ i ].camera, crude_transform, {
  //  .translation = { 0, 0, -5 },
  //  .rotation = { 0, 0, 0, 1 },
  //  .scale = { 1, 1, 1 }, } );
  //
  //CRUDE_ENTITY_SET_COMPONENT( renderer[ i ].camera, crude_free_camera, {
  //  .moving_speed_multiplier = { 7.0, 7.0, 7.0 },
  //  .rotating_speed_multiplier  = { -0.15f, -0.15f },
  //  .entity_input = ( crude_entity ){ it->entities[ i ], it->world } } );
  //
}

void
crude_sandbox_deinitialize
(
  _In_ crude_sandbox                                      *sandbox
)
{
  crude_heap_allocator_deinitialize( &sandbox->graphics_allocator );
  crude_stack_allocator_deinitialize( &sandbox->temporary_allocator );
}

bool
crude_sandbox_update
(
  _In_ crude_sandbox                                      *sandbox
)
{
}