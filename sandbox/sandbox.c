#include <cr/cr.h>
#include <assert.h>

#include <engine.h>
#include <platform/sdl_system.h>
#include <platform/input_components.h>
#include <platform/gui_components.h>
#include <graphics/render_components.h>
#include <graphics/render_system.h>
#include <core/memory.h>
#include <scene/free_camera_system.h>

static bool CR_STATE g_initialized = false;

bool
input_callback
(
  const void *sdl_event
)
{
}

// !TODO
void*
sandbox_allocate
(
  sizet size,
  sizet alignment
)
{
  return malloc( size );
}

void
sandbox_deallocate
(
  void* pointer
)
{
  free( pointer );
}

crude_heap_allocator gltf_allocator;
crude_heap_allocator renderer_allocator;
crude_stack_allocator  temporary_allocator;

CR_EXPORT int
cr_main
(
  struct cr_plugin *ctx,
  enum cr_op        operation
)
{
  assert( ctx );

  if ( operation == CR_CLOSE )
  {
    return 0;
  }

  ecs_world_t* world = ctx->userdata;

  if ( !g_initialized )
  {
    crude_heap_allocator_initialize( &gltf_allocator, CRUDE_RMEGA( 512 ), "GltfAllocator" );
    crude_heap_allocator_initialize( &renderer_allocator, CRUDE_RMEGA( 32 ), "RendererAllocator" );
    crude_stack_allocator_initialize( &temporary_allocator, CRUDE_RMEGA( 32 ), "TemproraryAllocator" );
    ECS_IMPORT( world, crude_sdl_system );
    ECS_IMPORT( world, crude_render_system );
    ECS_IMPORT( world, crude_free_camera_system );
    
    ecs_entity_t scene = ecs_entity( world, { .name = "scene1" } );
    ecs_set( world, scene, crude_window, { 
      .width     = 800,
      .height    = 600,
      .maximized = false });
    ecs_set( world, scene, crude_input, { .callback = &input_callback } );
    ecs_set( world, scene, crude_render_create, {
      .vk_application_name = "sandbox",
      .vk_application_version = VK_MAKE_VERSION( 1, 0, 0 ),
      .allocator = crude_heap_allocator_pack( &renderer_allocator ),
      .gltf_allocator = crude_heap_allocator_pack( &gltf_allocator ),
      .temporary_allocator = &temporary_allocator,
      .max_frames = 3u } );
    g_initialized = true;
  }
 
  return 0;
}