#include <gui/gui.h>

ECS_COMPONENT_DECLARE( crude_gui_system );
ECS_COMPONENT_DECLARE( crude_gui_context );
ECS_COMPONENT_DECLARE( crude_window );
ECS_COMPONENT_DECLARE( crude_window_handle );

void crude_gui_componentsImport( crude_world* world )
{
  ECS_MODULE( world, crude_gui_components );

  ECS_COMPONENT_DEFINE( world, crude_gui_system );
  ECS_COMPONENT_DEFINE( world, crude_gui_context );
  ECS_COMPONENT_DEFINE( world, crude_window );
  ECS_COMPONENT_DEFINE( world, crude_window_handle );
  
  ecs_struct_init(world, &(ecs_struct_desc_t){
    .entity  = ecs_id( crude_window ),
    .members = {
      { .name = "width", .type = ecs_id( ecs_i32_t ) },
      { .name = "height", .type = ecs_id( ecs_i32_t ) },
      { .name = "maximized", .type = ecs_id( ecs_bool_t ) 
    } } } );
  
  ecs_struct_init(world, &( ecs_struct_desc_t ){
    .entity  = ecs_id( crude_window_handle ),
    .members = {
      {.name = "value", .type = sizeof( void * ) == 4 ? ecs_id( ecs_i32_t ) : ecs_id( ecs_i64_t ) }
    } } );
}
