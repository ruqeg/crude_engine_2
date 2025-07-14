#pragma once

#include <core/math.h>
#include <core/ecs.h>

typedef struct crude_free_camera
{
  XMFLOAT3                                                 moving_speed_multiplier;
  XMFLOAT2                                                 rotating_speed_multiplier;
  crude_entity                                             entity_input;
  bool                                                     enabled; // TODO?
} crude_free_camera;

CRUDE_API ECS_COMPONENT_DECLARE( crude_free_camera );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_scripts_components );