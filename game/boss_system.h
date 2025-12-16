#pragma once

#include <engine/core/ecs.h>
#include <engine/external/game_components.h>

CRUDE_API void
crude_boss_receive_damage
(
  _In_ crude_boss                                         *boss,
  _In_ crude_entity                                        node
);

CRUDE_ECS_MODULE_IMPORT_DECL( crude_boss_system );