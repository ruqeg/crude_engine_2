#include <engine.h>
#include <platform/platform_system.h>
#include <platform/platform_components.h>
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
}

bool
crude_sandbox_update
(
  _In_ crude_sandbox                                      *sandbox
)
{
}