#include <cr.h>
#include <assert.h>

#include <engine.h>
#include <platform/sdl_system.h>
#include <platform/input.h>
#include <graphics/render_core.h>
#include <graphics/render_system.h>
#include <gui/gui.h>

static bool CR_STATE g_initialized = false;

bool input_callback( const void *sdl_event )
{
}

CR_EXPORT int cr_main( struct cr_plugin *ctx, enum cr_op operation )
{
  if ( operation == CR_CLOSE )
    return 0;

  assert( ctx );

  ecs_world_t* world = ctx->userdata;

  if ( !g_initialized )
  {
    ECS_IMPORT( world, crude_sdl_system );
    ECS_IMPORT( world, crude_render_system );

    ecs_entity_t scene = ecs_entity( world, { .name = "scene1" } );
    ecs_set( world, scene, crude_window, { 
      .width     = 800,
      .height    = 600,
      .maximized = false });
    ecs_set( world, scene, crude_input, { .callback = &input_callback } );
    ecs_set( world, scene, crude_render_core_config, {
      .application_name = "sandbox",
      .application_version = VK_MAKE_VERSION( 1, 0, 0 ),
      .allocation_callbacks = NULL } );

    g_initialized = true;
  }
 
  return 0;
}