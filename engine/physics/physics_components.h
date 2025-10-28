#pragma once

#include <core/math.h>
#include <core/ecs.h>

typedef enum crude_physics_collision_shape_type
{
  CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_BOX,
  CRUDE_PHYSICS_COLLISION_SHAPE_TYPE_SPHERE,
} crude_physics_collision_shape_type;

typedef struct crude_physics_box_collision_shape
{
  XMFLOAT3                                                 half_extent;
} crude_physics_box_collision_shape;

typedef struct crude_physics_sphere_collision_shape
{
  float32                                                  radius;
} crude_physics_sphere_collision_shape;

typedef struct crude_physics_static_body
{
  union 
  {
    crude_physics_box_collision_shape                      box_shape;
    crude_physics_sphere_collision_shape                   sphere_shape;            
  };
  crude_physics_collision_shape_type                       collision_shape_type;
} crude_physics_static_body;

typedef struct crude_physics_static_body_handle
{
  uint32                                                   static_body_index;
} crude_physics_static_body_handle;

typedef struct crude_physics_dynamic_body
{
  union 
  {
    crude_physics_box_collision_shape                      box_shape;
    crude_physics_sphere_collision_shape                   sphere_shape;     
  };
  crude_physics_collision_shape_type                       collision_shape_type;
} crude_physics_dynamic_body;

typedef struct crude_physics_dynamic_body_handle
{
  uint32                                                   dynamic_body_index;
} crude_physics_dynamic_body_handle;

/************************************************
 *
 * ECS Components Declaration
 * 
 ***********************************************/
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_physics_static_body );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_physics_static_body_handle );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_physics_dynamic_body );
CRUDE_API extern ECS_COMPONENT_DECLARE( crude_physics_dynamic_body_handle );

/************************************************
 *
 * Functions Declaratin
 * 
 ***********************************************/
CRUDE_ECS_MODULE_IMPORT_DECL( crude_physics_components );
