#include <graphics/render_components.h>

ECS_COMPONENT_DECLARE( crude_render_create );
ECS_COMPONENT_DECLARE( crude_renderer );

void crude_render_componentsImport( ecs_world_t *world )
{
  ECS_MODULE( world, crude_render_components );
  ECS_COMPONENT_DEFINE( world, crude_render_create );
  ECS_COMPONENT_DEFINE( world, crude_renderer );
}