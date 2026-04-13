#pragma once

#include <engine/core/ecs.h>
#include <engine/scene/components_serialization.h>
#include <engine/audio/audio_device.h>

/**********************************************************
 *
 *                 Components
 *
 *********************************************************/
CRUDE_API ECS_COMPONENT_DECLARE( crude_audio_listener );
CRUDE_API ECS_COMPONENT_DECLARE( crude_audio_player );
CRUDE_API ECS_COMPONENT_DECLARE( crude_audio_player_handle );

CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_audio_listener );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_audio_listener );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_audio_player );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_audio_player );

CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_audio_listener );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_audio_player );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_audio_player_handle );

CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_audio_listener );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_audio_player );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_audio_player_handle );

CRUDE_API void
crude_audio_components_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager
);

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_audio_system_context
{
  crude_ecs                                               *world;
  crude_audio_device                                      *device;
} crude_audio_system_context;


CRUDE_API void
crude_audio_listener_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_audio_player_create_observer_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_audio_player_destroy_observer_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_audio_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_audio_system_context                         *ctx
);