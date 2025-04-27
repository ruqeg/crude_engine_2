#include <cr/cr.h>
#include <assert.h>

#include <engine.h>
#include <platform/sdl_system.h>
#include <platform/input_components.h>
#include <platform/gui_components.h>
#include <graphics/render_components.h>
#include <graphics/render_system.h>
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
      .allocator = ( crude_allocator ) { .allocate = sandbox_allocate, .deallocate = sandbox_deallocate },
      .max_frames = 3u } );
    g_initialized = true;
  }
 
  return 0;
}