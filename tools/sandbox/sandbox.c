#include <engine.h>
#include <core/ecs_utils.h>
#include <platform/platform_system.h>
#include <platform/platform_components.h>
#include <graphics/graphics_system.h>
#include <graphics/graphics_components.h>
#include <scene/free_camera_system.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>

#include <sandbox.h>

void
crude_sandbox_initialize
(
  _In_ crude_sandbox                                      *sandbox,
  _In_ crude_engine                                       *engine
)
{
  sandbox->engine = engine;

  crude_heap_allocator_initialize( &sandbox->graphics_allocator, CRUDE_RMEGA( 32 ), "renderer_allocator" );
  crude_stack_allocator_initialize( &sandbox->temporary_allocator, CRUDE_RMEGA( 32 ), "temprorary_allocator" );
  
  ECS_IMPORT( sandbox->engine->world, crude_platform_system );
  ECS_IMPORT( sandbox->engine->world, crude_graphics_system );
  ECS_IMPORT( sandbox->engine->world, crude_free_camera_system );
  
  sandbox->scene = crude_entity_create_empty( sandbox->engine->world, "sandbox" );

  /* Create free camera */
  sandbox->camera = crude_entity_create_empty( sandbox->engine->world, "camera1" );
  CRUDE_ENTITY_SET_COMPONENT( sandbox->camera, crude_camera, {
    .fov_radians = CRUDE_CPI4,
    .near_z = 0.01,
    .far_z = 1000,
    .aspect_ratio = 4.0 / 3.0 } );
  CRUDE_ENTITY_SET_COMPONENT( sandbox->camera, crude_transform, {
    .translation = { 0, 0, -5 },
    .rotation = { 0, 0, 0, 1 },
    .scale = { 1, 1, 1 }, } );
  CRUDE_ENTITY_SET_COMPONENT( sandbox->camera, crude_free_camera, {
    .moving_speed_multiplier = { 7.0, 7.0, 7.0 },
    .rotating_speed_multiplier  = { -0.25f, -0.25f },
    .entity_input = sandbox->scene } );
  
  /* Create scene */
  CRUDE_ENTITY_SET_COMPONENT( sandbox->scene, crude_window, { 
    .width     = 800,
    .height    = 600,
    .maximized = false });
  CRUDE_ENTITY_SET_COMPONENT( sandbox->scene, crude_scene, { 0 } );
  CRUDE_ENTITY_SET_COMPONENT( sandbox->scene, crude_input, { 0 } );
  CRUDE_ENTITY_SET_COMPONENT( sandbox->scene, crude_gfx_graphics_creation, {
    .task_sheduler       = sandbox->engine->task_sheduler,
    .allocator_container = crude_heap_allocator_pack( &sandbox->graphics_allocator ),
    .temporary_allocator = &sandbox->temporary_allocator,
    .camera              = sandbox->camera,
  } );
}

void
crude_sandbox_deinitialize
(
  _In_ crude_sandbox                                      *sandbox
)
{
  if ( !sandbox->working )
  {
    return;
  }
  sandbox->working = false;
  crude_entity_destroy( sandbox->camera );
  crude_entity_destroy( sandbox->scene );
  crude_heap_allocator_deinitialize( &sandbox->graphics_allocator );
  crude_stack_allocator_deinitialize( &sandbox->temporary_allocator );
}

bool
crude_sandbox_update
(
  _In_ crude_sandbox                                      *sandbox
)
{
  crude_input *input = CRUDE_ENTITY_GET_IMMUTABLE_COMPONENT( sandbox->scene, crude_input );
  if ( input && input->should_close_window )
  {
    crude_sandbox_deinitialize( sandbox );
  }
}