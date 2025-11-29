#pragma once

#include <engine/core/ecs.h>

CRUDE_API void
crude_player_update_system_
(
  _In_ ecs_iter_t                                         *it
);

CRUDE_ECS_MODULE_IMPORT_DECL( crude_player_system );