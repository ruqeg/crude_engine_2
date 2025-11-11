#pragma once

#include <core/ecs.h>
#include <core/math.h>

typedef struct crude_enemy
{
  float32                                                  weight;
  float32                                                  rotation_speed;

  XMFLOAT2                                                 moving_speed;

  crude_entity                                             entity_input;
  bool                                                     input_enabled;
} crude_enemy;

CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_enemy );
CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_enemy );
CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_enemy );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_enemy );

CRUDE_API ECS_COMPONENT_DECLARE( crude_enemy_components );

CRUDE_ECS_MODULE_IMPORT_DECL( crude_enemy_components );