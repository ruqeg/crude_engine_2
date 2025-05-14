#include <engine.h>
#include <core/ecs_utils.h>
#include <platform/platform_system.h>
#include <platform/platform_components.h>
#include <graphics/graphics_system.h>
#include <graphics/graphics_components.h>
#include <scene/free_camera_system.h>
#include <scene/scene_components.h>
#include <scene/scripts_components.h>

#include <paprika.h>

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
  ECS_IMPORT( paprika->engine->world, crude_graphics_system );
  ECS_IMPORT( paprika->engine->world, crude_free_camera_system );
  
  paprika->scene = crude_entity_create_empty( paprika->engine->world, "paprika" );

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
  CRUDE_ENTITY_SET_COMPONENT( paprika->scene, crude_gfx_graphics_creation, {
    .task_sheduler       = paprika->engine->task_sheduler,
    .allocator_container = crude_heap_allocator_pack( &paprika->graphics_allocator ),
    .temporary_allocator = &paprika->temporary_allocator,
    .camera              = paprika->camera,
  } );
}

void
crude_paprika_deinitialize
(
  _In_ crude_paprika                                      *paprika
)
{
  crude_entity_destroy( paprika->camera );
  crude_entity_destroy( paprika->scene );
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