#include <SDL3/SDL.h>
#include <stdio.h>
#include <engine.h>
#include <platform/sdl_system.h>
#include <platform/input_components.h>
#include <platform/gui_components.h>
#include <cr/cr.h>

static bool CR_STATE g_initialized = false;

bool
input_callback
(
  _In_ const void *sdl_event
)
{
}

CR_EXPORT int
cr_main
(
  _In_ struct cr_plugin *ctx,
  _In_ enum cr_op        operation
)
{
  assert( ctx );
  if ( operation != CR_STEP )
  {
    return 0;
  }
  
  ecs_world_t* world = ctx->userdata;

  if ( !g_initialized )
  {
    //ECS_IMPORT( world, crude_sdl_system );
    //
    //ecs_entity_t scene = ecs_entity( world, { .name = "paprika" } );
    //ecs_set( world, scene, crude_window, { 
    //  .width     = 800,
    //  .height    = 600,
    //  .maximized = false });
    //ecs_set( world, scene, crude_input, { .callback = &input_callback } );
    
    g_initialized = true;
  }

  return 0;
}