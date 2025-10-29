#pragma once

#include <core/ecs.h>

typedef struct crude_player_controller
{
  float32                                                  weight;
  float32                                                  speed;
  float32                                                  jump_velocity;
  float32                                                  rotation_speed;
  float32                                                  direction_change;
  float32                                                  model_direction_change;
} crude_player_controller;

CRUDE_API ECS_COMPONENT_DECLARE( crude_player_controller );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_crude_player_controller_components );