#include <core/assert.h>
#include <core/string.h>

#include <physics/physics_resource.h>

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
  _In_ crude_physics_collision_callback_container          container
)
{
  if ( container.fun )
  {
    container.fun( container.ctx );
  }
}