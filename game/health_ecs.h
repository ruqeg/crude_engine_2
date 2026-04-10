#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
typedef struct crude_health crude_health;

typedef void (*crude_health_damage_callback_fn)
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
);

typedef void (*crude_health_death_callback_fn)
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
);

typedef struct crude_health_callback_container
{
  crude_health_damage_callback_fn                          damage_callback;
  crude_health_death_callback_fn                           death_callback;
} crude_health_callback_container;

typedef struct crude_health
{
  crude_health_callback_container                          callback_container;
  int32                                                    max_health;
  int32                                                    total_damage;
} crude_health;

CRUDE_API ECS_COMPONENT_DECLARE( crude_health );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_health );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_health );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_health );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_health );

/**********************************************************
 *
 *                 API
 *
 *********************************************************/
CRUDE_API void
crude_heal_receive_damage 
(
  _In_ crude_entity                                        health_entity,
  _In_ crude_health                                       *health,
  _In_ int32                                               damage
);

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_health_system_context
{
} crude_health_system_context;

CRUDE_API void
crude_health_game_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_health_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_health_system_context                        *ctx
);