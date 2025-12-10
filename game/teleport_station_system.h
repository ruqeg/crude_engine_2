#pragma once

#include <engine/core/ecs.h>

CRUDE_API void
crude_teleport_station_set_serum
(
  _In_ crude_teleport_station                             *teleport_station,
  _In_ crude_player                                       *player,
  _In_ crude_entity                                        teleport_station_node
);

CRUDE_ECS_MODULE_IMPORT_DECL( crude_teleport_station_system );