#pragma once

#include <engine/core/ecs.h>
#include <engine/external/game_components.h>

CRUDE_API void
crude_enemy_deal_damage_to_player
(
	_In_ crude_enemy																				*enemy,
	_In_ crude_player																				*player
);

CRUDE_API void
crude_enemy_receive_damage
(
	_In_ crude_enemy																				*enemy,
	_In_ float64																				     damage,
  _In_ bool                                                critical
);

CRUDE_ECS_MODULE_IMPORT_DECL( crude_enemy_system );