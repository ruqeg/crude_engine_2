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
  
  ECS_IMPORT( world, crude_platform_system );
  ECS_IMPORT( world, crude_graphics_system );
  ECS_IMPORT( world, crude_free_camera_system );
  
  ecs_entity_t scene = ecs_entity( world, { .name = "scene1" } );
  ecs_set( world, scene, crude_window, { 
    .width     = 800,
    .height    = 600,
    .maximized = false });
  ecs_set( world, scene, crude_input, { 0 } );
  ecs_set( world, scene, crude_graphics_creation, {
    .task_sheduler       = task_sheduler,
    .allocator_container = crude_heap_allocator_pack( &renderer_allocator ),
    .temporary_allocator = &temporary_allocator,
  } );
  g_initialized = true;
}

void
crude_sandbox_deinitialize
(
  _In_ crude_sandbox                                      *sandbox
)
{
}