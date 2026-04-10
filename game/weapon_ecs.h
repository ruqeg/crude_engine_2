#pragma once

#include <engine/core/ecs.h>
#include <engine/platform/platform.h>
#include <engine/scene/components_serialization.h>

/**********************************************************
 *
 *                 Component
 *
 *********************************************************/
typedef struct crude_weapon
{
  int32                                                    max_ammo;
  int32                                                    wasted_ammo;
  int32                                                    damage;
  float32                                                  max_cooldown;
  float32                                                  cooldown;
} crude_weapon;

CRUDE_API ECS_COMPONENT_DECLARE( crude_weapon );
CRUDE_API CRUDE_COMPONENT_STRING_DECLARE( crude_weapon );
CRUDE_API CRUDE_PARSE_JSON_TO_COMPONENT_FUNC_DECLARATION( crude_weapon );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_JSON_FUNC_DECLARATION( crude_weapon );
CRUDE_API CRUDE_PARSE_COMPONENT_TO_IMGUI_FUNC_DECLARATION( crude_weapon );

/**********************************************************
 *
 *                 API
 *
 *********************************************************/
CRUDE_API void
crude_weapon_give_ammo
(
  _In_ crude_weapon                                       *weapon,
  _In_ int32                                               ammo
);

CRUDE_API void
crude_weapon_fill_ammo
(
  _In_ crude_weapon                                       *weapon
);

CRUDE_API bool
crude_weapon_fire
(
  _In_ crude_entity                                        weapon_entity,
  _In_ crude_weapon                                       *weapon
);

/**********************************************************
 *
 *                 System
 *
 *********************************************************/
typedef struct crude_weapon_system_context
{
} crude_weapon_system_context;

CRUDE_API void
crude_weapon_game_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_API void
crude_weapon_system_import
(
  _In_ crude_ecs                                          *world,
  _In_ crude_components_serialization_manager             *manager,
  _In_ crude_weapon_system_context                        *ctx
);