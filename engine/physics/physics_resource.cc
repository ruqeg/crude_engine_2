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
  else
  {
    CRUDE_ASSERT( false );
  }
}

char const*
crude_physics_collision_shape_get_debug_model_filename
(
  _In_ crude_physics_collision_shape_type                  type
)
{
  if ( type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX )
  {
    return "editor\\models\\crude_physics_box_collision_shape.gltf";
  }
  else if ( type == CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE )
  {
    return "editor\\models\\crude_physics_sphere_collision_shape.gltf";
  }
  else
  {
    CRUDE_ASSERT( false );
  }
}