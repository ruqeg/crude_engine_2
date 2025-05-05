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