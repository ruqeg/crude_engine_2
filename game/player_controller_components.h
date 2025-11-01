#pragma once

#include <core/ecs.h>
#include <core/components_serialization.h>

typedef struct crude_player_controller
{
  float32                                                  weight;
  float32                                                  speed;
  float32                                                  jump_velocity;
  float32                                                  rotation_speed;
  float32                                                  direction_change;
  float32                                                  model_direction_change;
} crude_player_controller;

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player_controller );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller );

void
crude_player_controller_parse_json_component
(
  _In_ void const                                         *component_json,
  _In_ char const                                         *component_name
);

void
crude_player_controller_save_component_to_json
(
  _In_ void                                               *node_components_json
);

CRUDE_API ECS_COMPONENT_DECLARE( crude_player_controller );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_crude_player_controller_components );