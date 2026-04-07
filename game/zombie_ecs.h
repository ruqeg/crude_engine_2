#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
typedef struct crude_zombie
{
  /* Joints */
  uint64                                                   head_joint_node;
  uint64                                                   spine_joint_node;

  /* Animations */
  uint32                                                   idle_animation_index;
} crude_zombie;

CRUDE_API ECS_COMPONENT_DECLARE( crude_zombie );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_zombie );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_zombie );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_zombie );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_zombie );

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_zombie_system_context
{
} crude_zombie_system_context;

CRUDE_API void
crude_zombie_game_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_zombie_engine_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_zombie_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_zombie_system_context                        *ctx
);