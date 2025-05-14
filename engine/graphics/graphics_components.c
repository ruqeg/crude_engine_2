#include <graphics/graphics_components.h>

#include <core/ecs_utils.h>

CRUDE_ECS_COMPONENT_DECLARE( crude_gfx_graphics_handle );
CRUDE_ECS_COMPONENT_DECLARE( crude_gfx_graphics_creation );

CRUDE_ECS_MODULE_IMPORT_IMPL( crude_graphics_components )
{
  ECS_MODULE( world, crude_graphics_components );
  ECS_COMPONENT_DEFINE( world, crude_gfx_graphics_handle );
  ECS_COMPONENT_DEFINE( world, crude_gfx_graphics_creation );
}