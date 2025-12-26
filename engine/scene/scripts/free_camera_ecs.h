#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
typedef struct crude_free_camera
{
  float32                                                  moving_speed_multiplier;
  float32                                                  rotating_speed_multiplier;
  crude_input const                                       *input;
  bool                                                     input_enabled;
} crude_free_camera;

CRUDE_API ECS_COMPONENT_DECLARE( crude_free_camera );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_free_camera );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_free_camera );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_free_camera );

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_free_camera_system_context
{
  crude_input const                                       *input;
} crude_free_camera_system_context;

CRUDE_API void
crude_free_camera_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_free_camera_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_free_camera_system_context                   *ctx
);