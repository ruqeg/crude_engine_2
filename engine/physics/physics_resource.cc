#include <engine/core/assert.h>
#include <engine/core/string.h>

#include <engine/physics/physics_resource.h>

char const*
crude_physics_collision_shape_type_to_string
(
  _In_ crude_physics_collision_shape_type                  type
)
{
  if ( type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
  {
    return "box";
  }
  else if ( type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
  {
    return "sphere";
  }
  else if ( type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH )
  {
    return "mesh";
  }
  else
  {
    CRUDE_ASSERT( false );
  }
}

crude_physics_collision_shape_type
crude_physics_collision_shape_string_to_type
(
  _In_ char const*                                         type_str
)
{
  if ( crude_string_cmp( type_str, "box" ) == 0 )
  {
    return CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX;
  }
  else if ( crude_string_cmp( type_str, "sphere" ) == 0 )
  {
    return CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE;
  }
  else if ( crude_string_cmp( type_str, "mesh" ) == 0 )
  {
    return CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_MESH;
  }
  else
  {
    CRUDE_ASSERT( false );
  }
}

crude_physics_collision_callback_container
crude_physics_collision_callback_container_empty
(
)
{
  crude_physics_collision_callback_container container = CRUDE_COMPOUNT_EMPTY( crude_physics_collision_callback_container );
  return container;
}

void
crude_physics_collision_callback_container_fun
(
  _In_ crude_physics_collision_callback_container          container,
  _In_ crude_entity                                        character_node,
  _In_ crude_entity                                        static_body_node,
  _In_ uint32                                              static_body_layer
)
{
  if ( container.fun )
  {
    container.fun( container.ctx, character_node, static_body_node, static_body_layer );
  }
}

crude_physics_character_body
crude_physics_character_body_empty
(  
)
{
  crude_physics_character_body body = CRUDE_COMPOUNT_EMPTY( crude_physics_character_body );
  return body;
}

CRUDE_API crude_physics_static_body
crude_physics_static_body_empty
(
)
{
  crude_physics_static_body body = CRUDE_COMPOUNT_EMPTY( crude_physics_static_body );
  return body;
}

CRUDE_API crude_physics_collision_shape
crude_physics_collision_shape_empty
(
)
{
  crude_physics_collision_shape shape = CRUDE_COMPOUNT_EMPTY( crude_physics_collision_shape );
  shape.type = CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX;
  shape.box.half_extent.x = 1;
  shape.box.half_extent.y = 1;
  shape.box.half_extent.z = 1;
  return shape;
}