#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>
#include <engine/audio/audio_ecs.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
typedef struct crude_maze_level
{
} crude_maze_level;

CRUDE_API ECS_COMPONENT_DECLARE( crude_maze_level );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_maze_level );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_maze_level );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_maze_level );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_maze_level );

/**/
CRUDE_API void
crude_key_sensor_callback_
(
  _In_ crude_entity                                        signal_entity,
  _In_ crude_entity                                        hitted_entity
);

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_maze_level_system_context
{
  crude_input const                                       *input;
} crude_maze_level_system_context;


CRUDE_API void
crude_maze_level_create_observer_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_maze_level_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_maze_level_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_maze_level_system_context                    *ctx
);