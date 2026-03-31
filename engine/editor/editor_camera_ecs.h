#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
typedef struct crude_editor_camera
{
  float32                                                  walk_speed;
  float32                                                  rotate_speed;
  bool                                                     input_enabled;
  XMFLOAT2                                                 last_unrelative_mouse_position;
} crude_editor_camera;

CRUDE_API ECS_COMPONENT_DECLARE( crude_editor_camera );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_editor_camera );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_editor_camera );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_editor_camera );

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_editor_camera_system_context
{
  crude_platform                                          *platform;
} crude_editor_camera_system_context;

CRUDE_API void
crude_editor_camera_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_editor_camera_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_editor_camera_system_context                 *ctx
);