#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
typedef struct crude_player_controller
{
  float32                                                  walk_speed;
  float32                                                  rotate_speed;
  crude_input const                                       *input;
  float32                                                  pitch_limit;
  bool                                                     input_enabled;
  bool                                                     camera_enabled;
} crude_player_controller;

CRUDE_API ECS_COMPONENT_DECLARE( crude_player_controller );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_player_controller );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_player_controller );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_player_controller );

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_player_controller_system_context
{
  crude_input const                                       *input;
} crude_player_controller_system_context;

CRUDE_API void
crude_player_controller_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_player_controller_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_player_controller_system_context             *ctx
);